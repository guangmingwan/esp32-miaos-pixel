#ifndef _SDL_CDROM_H
#define _SDL_CDROM_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAX_TRACKS 99

typedef struct SDL_CD {
	int         id;
	SDL_bool    in_use;
	int         status;
	int         numtracks;
	int         cur_track;
	int         cur_frame;
	struct {
		Uint8 type;
		Uint16 unused;
		Uint32 length;
		Uint32 offset;
	} track[SDL_MAX_TRACKS + 1];
} SDL_CD;

#define SDL_CD_TRAYEMPTY    0
#define SDL_CD_STOPPED      1
#define SDL_CD_PLAYING      2
#define SDL_CD_PAUSED       3
#define SDL_CD_ERROR        -1

#define CD_AUDIO    0x00
#define CD_DATA     0x40

#define FRAMES_PER_SECOND 75
#define FRAMES_PER_MINUTE (FRAMES_PER_SECOND * 60)

extern int       SDL_CDNumDrives(void);
extern const char *SDL_CDName(int drive);
extern SDL_CD   *SDL_CDOpen(int drive);
extern void      SDL_CDClose(SDL_CD *cdrom);
extern int       SDL_CDStatus(SDL_CD *cdrom);
extern int       SDL_CDPlayTracks(SDL_CD *cdrom, int start_track, int start_frame,
                                  int ntracks, int nframes);
extern int       SDL_CDPlay(SDL_CD *cdrom, int start, int length);
extern int       SDL_CDStop(SDL_CD *cdrom);
extern int       SDL_CDPause(SDL_CD *cdrom);
extern int       SDL_CDResume(SDL_CD *cdrom);
extern int       SDL_CDEject(SDL_CD *cdrom);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_CDROM_H */
