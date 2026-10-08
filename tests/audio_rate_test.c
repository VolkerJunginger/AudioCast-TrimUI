/* Public generated sine waves; no game or BIOS. GPL-2.0-or-later. */
#include "../sync/mgba-audio.h"
#include <mgba-util/audio-buffer.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int16_t input[16384], output[16384];
int main(void) {
    struct mAudioBuffer source;
    mAudioBufferInit(&source,16384,2);
    AudioCastAudioInit(&source,32768);
    const unsigned rates[]={32768,65536,131072,262144,32768};
    for(unsigned r=0;r<5;r++) {
        AudioCastAudioReset();AudioCastAudioRate(rates[r]);
        size_t frames=0;unsigned crossings=0;int16_t previous=0;
        for(unsigned block=0;block<64;block++) {
            unsigned count=rates[r]/64;
            for(unsigned i=0;i<count;i++) {
                int16_t v=(int16_t)(12000*sin(2*3.141592653589793*1000*(block*count+i)/(double)rates[r]));
                input[2*i]=v;input[2*i+1]=-v;
            }
            assert(mAudioBufferWrite(&source,input,count)==count);
            AudioCastAudioProcess();size_t n=AudioCastAudioRead(output,8192);frames+=n;
            for(size_t i=0;i<n;i++) {
                if(previous<0 && output[2*i]>=0)crossings++;
                previous=output[2*i];
                assert(abs(output[2*i]+output[2*i+1])<=2);
            }
        }
        printf("DAC %u Hz -> 48000 Hz: %zu samples, %u 1kHz cycles\n",rates[r],frames,crossings);
        assert(frames>47950 && frames<=48000);
        assert(crossings>=998 && crossings<=1001);
    }
    AudioCastAudioDeinit();AudioCastAudioDeinit();mAudioBufferDeinit(&source);
    puts("PASS: all GBA DAC rates, pitch, stereo, reset and repeated cleanup");
}
