/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ESP32-S3 port: no separate audio thread — the SDL callback stored
// here is invoked from mia_sdl_audio_fill() inside mia_present_screen()
// once per video frame (~50 fps).  The callback mixes music + sound
// into an S16 stereo buffer, which the SDL layer pushes to I2S.
//

#include "audio.h"
#include "util.h"

AUDIODEVICE gAudioDevice;

static AUDIOPLAYER *g_pSoundPlayer = NULL;

//
// The SDL audio callback.  Called from mia_sdl_audio_fill() every frame.
// Mixes music and sound into the output buffer.
//
static void
AUDIO_FillBuffer(
	void     *userdata,
	Uint8   *stream,
	int      len
)
{
	AUDIOPLAYER *pMusPlayer = gAudioDevice.pMusPlayer;

	(void)userdata;

	//
	// Clear the buffer to silence (0x00 for signed 16-bit)
	//
	memset(stream, 0, (size_t)len);

	//
	// Mix music
	//
	if (gAudioDevice.fMusicEnabled && pMusPlayer && pMusPlayer->FillBuffer)
	{
		pMusPlayer->FillBuffer(pMusPlayer, stream, len);
	}

	//
	// Mix sound effects
	//
	if (gAudioDevice.fSoundEnabled && g_pSoundPlayer && g_pSoundPlayer->FillBuffer)
	{
		g_pSoundPlayer->FillBuffer(g_pSoundPlayer, stream, len);
	}
}

INT
AUDIO_OpenDevice(
	VOID
)
{
	//
	// Set up the desired audio spec: 44100 Hz, stereo, signed 16-bit LE
	//
	memset(&gAudioDevice, 0, sizeof(gAudioDevice));
	gAudioDevice.spec.freq     = 44100;
	gAudioDevice.spec.format   = AUDIO_S16SYS;
	gAudioDevice.spec.channels = 2;
	gAudioDevice.spec.samples  = 1024;
	gAudioDevice.spec.size     = gAudioDevice.spec.samples * gAudioDevice.spec.channels * sizeof(Sint16);
	gAudioDevice.spec.callback = AUDIO_FillBuffer;
	gAudioDevice.spec.userdata = NULL;
	gAudioDevice.iMusicVolume  = SDL_MIX_MAXVOLUME;
	gAudioDevice.iSoundVolume  = SDL_MIX_MAXVOLUME;
	gAudioDevice.fMusicEnabled = FALSE;
	gAudioDevice.fSoundEnabled = FALSE;
	gAudioDevice.fOpened       = FALSE;

	//
	// Open the SDL audio device (which opens I2S on ESP32)
	//
	if (SDL_OpenAudio(&gAudioDevice.spec, NULL) < 0)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_OpenAudio failed\n");
		return -1;
	}

	gAudioDevice.fOpened = TRUE;

	//
	// Initialize the RIX music player
	//
	{
		char mus_path[PAL_MAX_PATH];
		snprintf(mus_path, sizeof(mus_path), "%s/mus.mkf", PAL_PREFIX);
		gAudioDevice.pMusPlayer = RIX_Init(mus_path);
	}
	if (gAudioDevice.pMusPlayer == NULL)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "RIX_Init failed (mus.mkf not found?)\n");
	}

	//
	// Initialize the sound effects player
	//
	g_pSoundPlayer = SOUND_Init();
	if (g_pSoundPlayer == NULL)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "SOUND_Init failed\n");
	}

	//
	// Start audio playback (unpause)
	//
	SDL_PauseAudio(0);

	UTIL_LogOutput(LOGLEVEL_INFO, "AUDIO_OpenDevice: %d Hz %d ch\n",
		gAudioDevice.spec.freq, gAudioDevice.spec.channels);

	return 0;
}

VOID
AUDIO_CloseDevice(
	VOID
)
{
	if (!gAudioDevice.fOpened)
	{
		return;
	}

	SDL_PauseAudio(1);

	//
	// Shut down the music player
	//
	if (gAudioDevice.pMusPlayer)
	{
		if (gAudioDevice.pMusPlayer->Shutdown)
		{
			gAudioDevice.pMusPlayer->Shutdown(gAudioDevice.pMusPlayer);
		}
		gAudioDevice.pMusPlayer = NULL;
	}

	//
	// Shut down the sound player
	//
	if (g_pSoundPlayer)
	{
		if (g_pSoundPlayer->Shutdown)
		{
			g_pSoundPlayer->Shutdown(g_pSoundPlayer);
		}
		g_pSoundPlayer = NULL;
	}

	SDL_CloseAudio();

	gAudioDevice.fOpened = FALSE;
}

BOOL
AUDIO_CD_Available(
	VOID
)
{
	return FALSE;
}

SDL_AudioSpec *
AUDIO_GetDeviceSpec(
	VOID
)
{
	return &gAudioDevice.spec;
}

VOID
AUDIO_IncreaseVolume(
	VOID
)
{
	if (gAudioDevice.iMusicVolume < SDL_MIX_MAXVOLUME)
	{
		gAudioDevice.iMusicVolume++;
	}
}

VOID
AUDIO_DecreaseVolume(
	VOID
)
{
	if (gAudioDevice.iMusicVolume > 0)
	{
		gAudioDevice.iMusicVolume--;
	}
}

VOID
AUDIO_PlaySound(
	INT    iSoundNum
)
{
	if (!gAudioDevice.fOpened || !gAudioDevice.fSoundEnabled)
	{
		return;
	}

	if (g_pSoundPlayer && g_pSoundPlayer->Play)
	{
		g_pSoundPlayer->Play(g_pSoundPlayer, iSoundNum, FALSE, 0);
	}
}

VOID
AUDIO_PlayMusic(
	INT       iNumRIX,
	BOOL      fLoop,
	FLOAT     flFadeTime
)
{
	if (!gAudioDevice.fOpened)
	{
		return;
	}

	if (gAudioDevice.pMusPlayer && gAudioDevice.pMusPlayer->Play)
	{
		gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
	}
}

BOOL
AUDIO_PlayCDTrack(
	INT    iNumTrack
)
{
	(void)iNumTrack;
	return FALSE;
}

VOID
AUDIO_EnableMusic(
	BOOL   fEnable
)
{
	gAudioDevice.fMusicEnabled = fEnable;
}

BOOL
AUDIO_MusicEnabled(
	VOID
)
{
	return gAudioDevice.fMusicEnabled;
}

VOID
AUDIO_EnableSound(
	BOOL   fEnable
)
{
	gAudioDevice.fSoundEnabled = fEnable;
}

BOOL
AUDIO_SoundEnabled(
	VOID
)
{
	return gAudioDevice.fSoundEnabled;
}

void
AUDIO_Lock(
	void
)
{
	SDL_LockAudio();
}

void
AUDIO_Unlock(
	void
)
{
	SDL_UnlockAudio();
}
