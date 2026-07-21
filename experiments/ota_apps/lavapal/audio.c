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
// ESP32-S3 port: Core 0 continuously mixes music into I2S while the game
// and SDL rendering remain on Core 1.
//

#include "audio.h"
#include "palcfg.h"
#include "util.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "mia_host_abi.h"

AUDIODEVICE gAudioDevice;

static AUDIOPLAYER *g_pSoundPlayer = NULL;
static TaskHandle_t g_audioTask = NULL;
static SemaphoreHandle_t g_audioMutex = NULL;
static volatile BOOL g_audioRunning = FALSE;
static Uint8 *g_audioBuffer = NULL;

static void AUDIO_FillBuffer(void *userdata, Uint8 *stream, int len);

static void
AUDIO_Task(
	void *userdata
)
{
	(void)userdata;
	UTIL_LogOutput(LOGLEVEL_INFO, "AUDIO task started on core %d\n", xPortGetCoreID());
	while (g_audioRunning)
	{
		AUDIO_Lock();
		AUDIO_FillBuffer(NULL, g_audioBuffer, gAudioDevice.spec.size);
		AUDIO_Unlock();
		if (mia_host_audio_write_pcm16((const int16_t *)g_audioBuffer,
			gAudioDevice.spec.samples, gAudioDevice.spec.channels) < 0)
		{
			vTaskDelay(pdMS_TO_TICKS(1));
		}
	}
	g_audioTask = NULL;
	vTaskDelete(NULL);
}

//
// Mix music and sound into one signed 16-bit output buffer.
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
	// Set up signed 16-bit PCM using the same rate and channel count as RIX.
	//
	memset(&gAudioDevice, 0, sizeof(gAudioDevice));
	gAudioDevice.spec.freq     = gConfig.iSampleRate;
	gAudioDevice.spec.format   = AUDIO_S16SYS;
	gAudioDevice.spec.channels = gConfig.iAudioChannels;
	gAudioDevice.spec.samples  = gConfig.wAudioBufferSize;
	gAudioDevice.spec.size     = gAudioDevice.spec.samples * gAudioDevice.spec.channels * sizeof(Sint16);
	gAudioDevice.spec.callback = AUDIO_FillBuffer;
	gAudioDevice.spec.userdata = NULL;
	gAudioDevice.iMusicVolume  = SDL_MIX_MAXVOLUME;
	gAudioDevice.iSoundVolume  = SDL_MIX_MAXVOLUME;
	gAudioDevice.fMusicEnabled = TRUE;
	gAudioDevice.fSoundEnabled = TRUE;
	gAudioDevice.fOpened       = FALSE;

	//
	// Open I2S directly; Core 0 continuously pumps audio independently of video.
	//
	g_audioBuffer = malloc(gAudioDevice.spec.size);
	g_audioMutex = xSemaphoreCreateMutex();
	if (g_audioBuffer == NULL || g_audioMutex == NULL ||
		!mia_host_audio_open(gAudioDevice.spec.freq, gAudioDevice.spec.channels, 16))
	{
		free(g_audioBuffer);
		g_audioBuffer = NULL;
		if (g_audioMutex) vSemaphoreDelete(g_audioMutex);
		g_audioMutex = NULL;
		UTIL_LogOutput(LOGLEVEL_WARNING, "I2S audio open failed\n");
		return -1;
	}

	gAudioDevice.fOpened = TRUE;

	//
	// Initialize the RIX music player
	//
	{
		char mus_path[PAL_MAX_PATH];
		snprintf(mus_path, sizeof(mus_path), "%s/mus.mkf", gConfig.pszGamePath);
		gAudioDevice.pMusPlayer = RIX_Init(mus_path);
	}
	if (gAudioDevice.pMusPlayer == NULL)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "RIX_Init failed (mus.mkf not found or load error)\n");
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
	// Start the independent audio task after all players are initialized.
	//
	g_audioRunning = TRUE;
	if (xTaskCreatePinnedToCore(AUDIO_Task, "pal_audio", 8192, NULL, 6,
		&g_audioTask, 0) != pdPASS)
	{
		g_audioRunning = FALSE;
		mia_host_audio_close();
		free(g_audioBuffer);
		g_audioBuffer = NULL;
		vSemaphoreDelete(g_audioMutex);
		g_audioMutex = NULL;
		gAudioDevice.fOpened = FALSE;
		return -1;
	}

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

	g_audioRunning = FALSE;
	for (unsigned wait = 0; g_audioTask != NULL && wait < 300; ++wait)
	{
		vTaskDelay(pdMS_TO_TICKS(1));
	}
	if (g_audioTask != NULL)
	{
		vTaskDelete(g_audioTask);
		g_audioTask = NULL;
	}

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

	mia_host_audio_close();
	free(g_audioBuffer);
	g_audioBuffer = NULL;
	if (g_audioMutex) vSemaphoreDelete(g_audioMutex);
	g_audioMutex = NULL;

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
		AUDIO_Lock();
		gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
		AUDIO_Unlock();
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
	if (g_audioMutex) xSemaphoreTake(g_audioMutex, portMAX_DELAY);
}

void
AUDIO_Unlock(
	void
)
{
	if (g_audioMutex) xSemaphoreGive(g_audioMutex);
}
