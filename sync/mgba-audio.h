/* Fixed-rate private GBA libretro audio; MPL-2.0. */
#ifndef AUDIOCAST_MGBA_AUDIO_H
#define AUDIOCAST_MGBA_AUDIO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define AUDIOCAST_AUDIO_RATE 48000
struct mAudioBuffer;
void AudioCastAudioInit(struct mAudioBuffer*, unsigned rate);
bool AudioCastAudioActive(void);
void AudioCastAudioRate(unsigned rate);
void AudioCastAudioProcess(void);
size_t AudioCastAudioRead(int16_t*, size_t frames);
void AudioCastAudioReset(void);
void AudioCastAudioDeinit(void);
#endif
