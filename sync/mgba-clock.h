/* AudioCast virtual clock peripheral; MPL-2.0. */
#ifndef AUDIOCAST_MGBA_CLOCK_H
#define AUDIOCAST_MGBA_CLOCK_H
#include <stdint.h>
struct mCore;
void AudioCastClockAttach(struct mCore* core);
uint16_t AudioCastClockInput(struct mCore* core, uint16_t keys);
void AudioCastClockFrame(struct mCore* core);
void AudioCastClockReady(struct mCore* core);
void AudioCastClockFrameAt(struct mCore* core, int64_t host_us);
void AudioCastClockRebase(struct mCore* core);
void AudioCastClockDetach(struct mCore* core);
#endif
