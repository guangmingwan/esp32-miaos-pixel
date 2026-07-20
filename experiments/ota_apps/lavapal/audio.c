#include "audio.h"

AUDIODEVICE gAudioDevice;

INT AUDIO_OpenDevice(VOID)
{
	memset(&gAudioDevice, 0, sizeof(gAudioDevice));
	gAudioDevice.spec.freq = 44100;
	gAudioDevice.spec.channels = 2;
	gAudioDevice.fMusicEnabled = FALSE;
	gAudioDevice.fSoundEnabled = FALSE;
	gAudioDevice.fOpened = FALSE;
	return 0;
}

VOID AUDIO_CloseDevice(VOID) {}

BOOL AUDIO_CD_Available(VOID) { return FALSE; }

SDL_AudioSpec *AUDIO_GetDeviceSpec(VOID) { return &gAudioDevice.spec; }

VOID AUDIO_IncreaseVolume(VOID) {}
VOID AUDIO_DecreaseVolume(VOID) {}

VOID AUDIO_PlaySound(INT iSoundNum) { (void)iSoundNum; }

VOID AUDIO_PlayMusic(INT iNumRIX, BOOL fLoop, FLOAT flFadeTime)
{
	(void)iNumRIX;
	(void)fLoop;
	(void)flFadeTime;
}

BOOL AUDIO_PlayCDTrack(INT iNumTrack) { (void)iNumTrack; return FALSE; }

VOID AUDIO_EnableMusic(BOOL fEnable) { (void)fEnable; }
BOOL AUDIO_MusicEnabled(VOID) { return FALSE; }
VOID AUDIO_EnableSound(BOOL fEnable) { (void)fEnable; }
BOOL AUDIO_SoundEnabled(VOID) { return FALSE; }

void AUDIO_Lock(void) {}
void AUDIO_Unlock(void) {}
