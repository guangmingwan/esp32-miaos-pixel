#include "players.h"

#include "audio.h"
#include "palcfg.h"
#include "palcommon.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOUND_MAX_VOICES 8
#define SOUND_POSITION_SHIFT 16

typedef enum tagSOUNDFORMAT
{
	SOUND_FORMAT_U8,
	SOUND_FORMAT_S16
} SOUNDFORMAT;

typedef struct tagSOUNDSPEC
{
	const BYTE *samples;
	DWORD frames;
	DWORD frequency;
	BYTE channels;
	SOUNDFORMAT format;
} SOUNDSPEC;

typedef BOOL (*SOUNDLOADER)(const BYTE *, DWORD, SOUNDSPEC *);

typedef struct tagSOUNDVOICE
{
	BYTE *buffer;
	SOUNDSPEC spec;
	uint64_t position;
	DWORD step;
	DWORD sequence;
	INT sound_num;
} SOUNDVOICE;

typedef struct tagSOUNDPLAYER
{
	AUDIOPLAYER_COMMONS;
	FILE *mkf;
	SOUNDLOADER loader;
	SOUNDVOICE voices[SOUND_MAX_VOICES];
	DWORD next_sequence;
} SOUNDPLAYER, *LPSOUNDPLAYER;

static WORD
SOUND_ReadLE16(
	const BYTE *p
)
{
	return (WORD)(p[0] | ((WORD)p[1] << 8));
}

static DWORD
SOUND_ReadLE24(
	const BYTE *p
)
{
	return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16);
}

static DWORD
SOUND_ReadLE32(
	const BYTE *p
)
{
	return (DWORD)p[0] | ((DWORD)p[1] << 8) |
		((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static BOOL
SOUND_LoadVOC(
	const BYTE *data,
	DWORD len,
	SOUNDSPEC *spec
)
{
	DWORD cursor;

	if (len < 26 || memcmp(data, "Creative Voice File\x1a", 20) != 0)
	{
		return FALSE;
	}

	cursor = SOUND_ReadLE16(data + 20);
	if (cursor < 26 || cursor >= len)
	{
		return FALSE;
	}

	while (cursor < len)
	{
		BYTE block_type;
		DWORD block_len;
		const BYTE *block;

		block_type = data[cursor++];
		if (block_type == 0)
		{
			break;
		}
		if (cursor + 3 > len)
		{
			return FALSE;
		}
		block_len = SOUND_ReadLE24(data + cursor);
		cursor += 3;
		if (block_len > len - cursor)
		{
			return FALSE;
		}
		block = data + cursor;

		if (block_type == 1 && block_len >= 2 && block[1] == 0)
		{
			spec->samples = block + 2;
			spec->frames = block_len - 2;
			spec->frequency = 1000000u / (256u - block[0]);
			spec->channels = 1;
			spec->format = SOUND_FORMAT_U8;
			return spec->frames > 0 && spec->frequency > 0;
		}

		if (block_type == 9 && block_len >= 12)
		{
			DWORD frequency = SOUND_ReadLE32(block);
			BYTE bits = block[4];
			BYTE channels = block[5];
			WORD codec = SOUND_ReadLE16(block + 6);
			DWORD bytes_per_frame;

			if ((channels != 1 && channels != 2) || frequency == 0 ||
				!((bits == 8 && codec == 0) || (bits == 16 && codec == 4)))
			{
				cursor += block_len;
				continue;
			}
			bytes_per_frame = channels * (bits / 8);
			spec->samples = block + 12;
			spec->frames = (block_len - 12) / bytes_per_frame;
			spec->frequency = frequency;
			spec->channels = channels;
			spec->format = bits == 16 ? SOUND_FORMAT_S16 : SOUND_FORMAT_U8;
			return spec->frames > 0;
		}

		cursor += block_len;
	}

	return FALSE;
}

static BOOL
SOUND_LoadWAV(
	const BYTE *data,
	DWORD len,
	SOUNDSPEC *spec
)
{
	DWORD cursor = 12;
	DWORD frequency = 0;
	WORD format = 0;
	WORD channels = 0;
	WORD bits = 0;
	const BYTE *samples = NULL;
	DWORD sample_bytes = 0;

	if (len < 12 || memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0)
	{
		return FALSE;
	}

	while (cursor + 8 <= len)
	{
		const BYTE *chunk = data + cursor;
		DWORD chunk_len = SOUND_ReadLE32(chunk + 4);
		cursor += 8;
		if (chunk_len > len - cursor)
		{
			return FALSE;
		}

		if (memcmp(chunk, "fmt ", 4) == 0 && chunk_len >= 16)
		{
			format = SOUND_ReadLE16(data + cursor);
			channels = SOUND_ReadLE16(data + cursor + 2);
			frequency = SOUND_ReadLE32(data + cursor + 4);
			bits = SOUND_ReadLE16(data + cursor + 14);
		}
		else if (memcmp(chunk, "data", 4) == 0)
		{
			samples = data + cursor;
			sample_bytes = chunk_len;
		}

		cursor += chunk_len + (chunk_len & 1u);
	}

	if (format != 1 || (channels != 1 && channels != 2) ||
		(bits != 8 && bits != 16) || frequency == 0 || samples == NULL)
	{
		return FALSE;
	}

	spec->samples = samples;
	spec->frames = sample_bytes / (channels * (bits / 8));
	spec->frequency = frequency;
	spec->channels = (BYTE)channels;
	spec->format = bits == 16 ? SOUND_FORMAT_S16 : SOUND_FORMAT_U8;
	return spec->frames > 0;
}

static VOID
SOUND_ReleaseVoice(
	SOUNDVOICE *voice
)
{
	free(voice->buffer);
	memset(voice, 0, sizeof(*voice));
}

static INT
SOUND_ReadSample(
	const SOUNDVOICE *voice,
	DWORD frame,
	BYTE channel
)
{
	DWORD index = frame * voice->spec.channels + channel;

	if (voice->spec.format == SOUND_FORMAT_U8)
	{
		return ((INT)voice->spec.samples[index] - 128) << 8;
	}
	else
	{
		const BYTE *p = voice->spec.samples + index * 2;
		return (int16_t)SOUND_ReadLE16(p);
	}
}

static INT
SOUND_ClampSample(
	INT sample
)
{
	if (sample > 32767)
	{
		return 32767;
	}
	if (sample < -32768)
	{
		return -32768;
	}
	return sample;
}

static BOOL
SOUND_Play(
	VOID *object,
	INT sound_num,
	BOOL loop,
	FLOAT fade_time
)
{
	LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
	const SDL_AudioSpec *device = AUDIO_GetDeviceSpec();
	SOUNDSPEC spec;
	SOUNDVOICE *voice = NULL;
	BYTE *buffer;
	INT len;
	INT i;

	(void)loop;
	(void)fade_time;
	if (player == NULL || device->freq <= 0 || sound_num == 0)
	{
		return FALSE;
	}
	if (sound_num < 0)
	{
		sound_num = -sound_num;
	}
	AUDIO_Lock();
	for (i = 0; i < SOUND_MAX_VOICES; ++i)
	{
		if (player->voices[i].buffer != NULL &&
			player->voices[i].sound_num == sound_num)
		{
			AUDIO_Unlock();
			return FALSE;
		}
	}
	AUDIO_Unlock();

	len = PAL_MKFGetChunkSize(sound_num, player->mkf);
	if (len <= 0)
	{
		return FALSE;
	}
	buffer = (BYTE *)malloc((size_t)len);
	if (buffer == NULL)
	{
		return FALSE;
	}
	if (PAL_MKFReadChunk(buffer, len, sound_num, player->mkf) != len ||
		!player->loader(buffer, (DWORD)len, &spec))
	{
		free(buffer);
		return FALSE;
	}

	AUDIO_Lock();
	for (i = 0; i < SOUND_MAX_VOICES; ++i)
	{
		if (player->voices[i].buffer == NULL)
		{
			voice = &player->voices[i];
			break;
		}
		if (voice == NULL || player->voices[i].sequence < voice->sequence)
		{
			voice = &player->voices[i];
		}
	}
	SOUND_ReleaseVoice(voice);
	voice->buffer = buffer;
	voice->spec = spec;
	voice->position = 0;
	voice->step = (DWORD)(((uint64_t)spec.frequency << SOUND_POSITION_SHIFT) /
		(DWORD)device->freq);
	if (voice->step == 0)
	{
		voice->step = 1;
	}
	voice->sequence = ++player->next_sequence;
	voice->sound_num = sound_num;
	AUDIO_Unlock();
	return TRUE;
}

static VOID
SOUND_FillBuffer(
	VOID *object,
	LPBYTE stream,
	INT len
)
{
	LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
	const SDL_AudioSpec *device = AUDIO_GetDeviceSpec();
	INT output_channels = device->channels;
	INT output_frames;
	int16_t *output = (int16_t *)stream;
	INT i;

	if (player == NULL || (output_channels != 1 && output_channels != 2))
	{
		return;
	}
	output_frames = len / (output_channels * (INT)sizeof(int16_t));

	for (i = 0; i < SOUND_MAX_VOICES; ++i)
	{
		SOUNDVOICE *voice = &player->voices[i];
		INT frame;

		if (voice->buffer == NULL)
		{
			continue;
		}

		for (frame = 0; frame < output_frames; ++frame)
		{
			DWORD source_frame = (DWORD)(voice->position >> SOUND_POSITION_SHIFT);
			INT left;
			INT right;

			if (source_frame >= voice->spec.frames)
			{
				break;
			}
			left = SOUND_ReadSample(voice, source_frame, 0);
			right = voice->spec.channels == 2 ?
				SOUND_ReadSample(voice, source_frame, 1) : left;

			if (output_channels == 1)
			{
				INT mixed = output[frame] + ((left + right) / 2);
				output[frame] = (int16_t)SOUND_ClampSample(mixed);
			}
			else
			{
				INT index = frame * 2;
				output[index] = (int16_t)SOUND_ClampSample(output[index] + left);
				output[index + 1] = (int16_t)SOUND_ClampSample(output[index + 1] + right);
			}
			voice->position += voice->step;
		}

		if ((voice->position >> SOUND_POSITION_SHIFT) >= voice->spec.frames)
		{
			SOUND_ReleaseVoice(voice);
		}
	}
}

static VOID
SOUND_Shutdown(
	VOID *object
)
{
	LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
	INT i;

	if (player == NULL)
	{
		return;
	}
	for (i = 0; i < SOUND_MAX_VOICES; ++i)
	{
		SOUND_ReleaseVoice(&player->voices[i]);
	}
	if (player->mkf != NULL)
	{
		fclose(player->mkf);
	}
	free(player);
}

LPAUDIOPLAYER
SOUND_Init(
	VOID
)
{
	static const char *const dos_files[] = { "voc.mkf", "sounds.mkf" };
	static const char *const win_files[] = { "sounds.mkf", "voc.mkf" };
	const char *const *files = gConfig.fIsWIN95 ? win_files : dos_files;
	INT i;

	for (i = 0; i < 2; ++i)
	{
		FILE *mkf = UTIL_OpenFile(files[i]);
		LPSOUNDPLAYER player;

		if (mkf == NULL)
		{
			continue;
		}
		player = (LPSOUNDPLAYER)calloc(1, sizeof(SOUNDPLAYER));
		if (player == NULL)
		{
			fclose(mkf);
			return NULL;
		}
		player->Play = SOUND_Play;
		player->FillBuffer = SOUND_FillBuffer;
		player->Shutdown = SOUND_Shutdown;
		player->mkf = mkf;
		player->loader = i == 0 ?
			(gConfig.fIsWIN95 ? SOUND_LoadWAV : SOUND_LoadVOC) :
			(gConfig.fIsWIN95 ? SOUND_LoadVOC : SOUND_LoadWAV);
		UTIL_LogOutput(LOGLEVEL_INFO, "Sound effects loaded from %s\n", files[i]);
		return (LPAUDIOPLAYER)player;
	}

	return NULL;
}
