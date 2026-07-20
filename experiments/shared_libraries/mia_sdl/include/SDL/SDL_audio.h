#ifndef _SDL_AUDIO_H
#define _SDL_AUDIO_H

#include "SDL_types.h"
#include "SDL_rwops.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_U8      0x0008
#define AUDIO_S8      0x8008
#define AUDIO_U16LSB  0x0010
#define AUDIO_S16LSB  0x8010
#define AUDIO_U16MSB  0x1010
#define AUDIO_S16MSB  0x9010
#define AUDIO_U16     AUDIO_U16LSB
#define AUDIO_S16     AUDIO_S16LSB
#define AUDIO_U16SYS  AUDIO_U16LSB
#define AUDIO_S16SYS  AUDIO_S16LSB

#define SDL_AUDIO_BITSIZE(x)       (x & 0xFF)
#define SDL_AUDIO_ISFLOAT(x)       0
#define SDL_AUDIO_ISSIGNED(x)      (x & 0x8000)
#define SDL_AUDIO_ISUNSIGNED(x)    !SDL_AUDIO_ISSIGNED(x)
#define SDL_AUDIO_ISLITTLEENDIAN(x) (!(x & 0x1000))

#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE    0x00000001
#define SDL_AUDIO_ALLOW_FORMAT_CHANGE       0x00000002
#define SDL_AUDIO_ALLOW_CHANNELS_CHANGE     0x00000004
#define SDL_AUDIO_ALLOW_ANY_CHANGE          0x00000007

#define SDL_MIX_MAXVOLUME 128

typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef Uint16 SDL_AudioFormat;
typedef Uint32 SDL_AudioDeviceID;

typedef struct SDL_AudioSpec {
	int              freq;
	SDL_AudioFormat  format;
	Uint8            channels;
	Uint8            silence;
	Uint16           samples;
	Uint16           padding;
	Uint32           size;
	SDL_AudioCallback callback;
	void            *userdata;
} SDL_AudioSpec;

typedef struct SDL_AudioCVT {
	int         needed;
	SDL_AudioFormat src_format;
	SDL_AudioFormat dst_format;
	double      rate_incr;
	Uint8      *buf;
	int         len;
	int         len_cvt;
	int         len_mult;
	double      len_ratio;
	void (*filters[10])(struct SDL_AudioCVT *cvt, Uint16 format);
	int         filter_index;
} SDL_AudioCVT;

extern int   SDL_AudioInit(const char *driver_name);
extern void  SDL_AudioQuit(void);
extern int   SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
extern int   SDL_GetNumAudioDrivers(void);
extern const char *SDL_GetAudioDriver(int index);
extern const char *SDL_GetCurrentAudioDriver(void);
extern void  SDL_CloseAudio(void);
extern void  SDL_PauseAudio(int pause_on);
extern void  SDL_LockAudio(void);
extern void  SDL_UnlockAudio(void);
extern int   SDL_BuildAudioCVT(SDL_AudioCVT *cvt, SDL_AudioFormat src_format,
                               Uint8 src_channels, int src_rate,
                               SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate);
extern int   SDL_ConvertAudio(SDL_AudioCVT *cvt);
extern void  SDL_MixAudio(Uint8 *dst, const Uint8 *src, Uint32 len, int volume);

extern int             SDL_GetNumAudioDevices(int iscapture);
extern const char     *SDL_GetAudioDeviceName(int index, int iscapture);
extern SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture,
                                              const SDL_AudioSpec *desired,
                                              SDL_AudioSpec *obtained, int allowed_changes);
extern void            SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
extern void            SDL_LockAudioDevice(SDL_AudioDeviceID dev);
extern void            SDL_UnlockAudioDevice(SDL_AudioDeviceID dev);
extern void            SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_AUDIO_H */
