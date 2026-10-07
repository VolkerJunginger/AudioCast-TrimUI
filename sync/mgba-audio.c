/* Fixed-rate private GBA libretro audio.
 * This Source Code Form is subject to the Mozilla Public License, v. 2.0.
 * See https://mozilla.org/MPL/2.0/. */
#include "mgba-audio.h"
#include <mgba-util/audio-buffer.h>
#include <mgba-util/audio-resampler.h>
static struct mAudioResampler resampler;
static struct mAudioBuffer output;
static bool active;
void AudioCastAudioInit(struct mAudioBuffer* source, unsigned rate) {
    AudioCastAudioDeinit();
    mAudioBufferInit(&output, 8192, 2);
    mAudioResamplerInit(&resampler, mINTERPOLATOR_SINC);
    mAudioResamplerSetSource(&resampler, source, rate, true);
    mAudioResamplerSetDestination(&resampler, &output, AUDIOCAST_AUDIO_RATE);
    active = true;
}
bool AudioCastAudioActive(void) { return active; }
void AudioCastAudioProcess(void) { if (active) mAudioResamplerProcess(&resampler); }
void AudioCastAudioRate(unsigned rate) {
    if (!active || !rate || resampler.sourceRate == rate) return;
    /* Drain samples from the old DAC rate before interpreting new samples.
       Keep interpolation history for continuous audio; no frontend AV change. */
    AudioCastAudioProcess();
    resampler.sourceRate = rate;
}
size_t AudioCastAudioRead(int16_t* data, size_t frames) {
    return active ? mAudioBufferRead(&output, data, frames) : 0;
}
void AudioCastAudioReset(void) {
    if (!active) return;
    mAudioBufferClear(&output);
    mAudioBufferClear(resampler.source);
    resampler.timestamp = 0;
}
void AudioCastAudioDeinit(void) {
    if (!active) return;
    mAudioResamplerDeinit(&resampler);
    mAudioBufferDeinit(&output);
    active = false;
}
