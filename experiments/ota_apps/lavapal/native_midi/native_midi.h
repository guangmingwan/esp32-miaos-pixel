/*
 * Stub native_midi header. PAL_HAS_NATIVEMIDI is 0 in this port, so the
 * native MIDI functions are never invoked; midi.h still unconditionally
 * includes this file, so it must parse cleanly.
 */
#ifndef _NATIVE_MIDI_H_
#define _NATIVE_MIDI_H_

#include "SDL/SDL.h"

typedef struct _NativeMidiSong NativeMidiSong;

extern void native_midi_StubFunction(void);

#define native_midi_init()               (0)
#define native_midi_shutdown()
#define native_midi_loadsong(rw)         ((NativeMidiSong *)0)
#define native_midi_freesong(song)
#define native_midi_start(song)
#define native_midi_stop()
#define native_midi_pause()
#define native_midi_resume()
#define native_midi_setvolume(volume)
#define native_midi_setloops(loops)

#endif /* _NATIVE_MIDI_H_ */
