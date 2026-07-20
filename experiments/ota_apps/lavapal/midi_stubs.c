#include "players.h"
#include "common.h"

LPAUDIOPLAYER TIMIDITY_Init(VOID) { return NULL; }
LPAUDIOPLAYER TSF_Init(VOID) { return NULL; }
LPAUDIOPLAYER OGG_Init(VOID) { return NULL; }
LPAUDIOPLAYER OPUS_Init(VOID) { return NULL; }
LPAUDIOPLAYER MP3_Init(VOID) { return NULL; }

void MIDI_SetVolume(int iVolume) { (void)iVolume; }
void MIDI_Play(int iNumRIX, BOOL fLoop) { (void)iNumRIX; (void)fLoop; }
void MIDI_FillBuffer(LPBYTE stream, INT len) { (void)stream; (void)len; }
void MIDI_Shutdown(void) {}
