#include "audio.h"
#include <string.h>
#include <math.h>
#include <malloc.h>
#ifndef M_PI
#define M_PI 3.14159265358979f
#endif
#define SFX_RATE 22050
static u8  *s_voice = NULL;  static ndspWaveBuf s_vwb;
static s16 *s_shut = NULL;   static size_t s_shut_n = 0; static ndspWaveBuf s_shwb;
static s16 *s_proc = NULL;   static size_t s_proc_n = 0; static ndspWaveBuf s_pwb;
bool audio_init(void){
    if (R_FAILED(ndspInit())) return false;
    ndspSetOutputMode(NDSP_OUTPUT_MONO);
    for (int ch = 0; ch < 2; ch++){
        float mix[12] = {0};
        mix[0] = mix[1] = 1.0f;
        ndspChnReset(ch);
        ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
        ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
        ndspChnSetRate(ch, (float)SFX_RATE);
        ndspChnSetMix(ch, mix);
    }
    // shutter: ~70ms click (tone + clicky edge, fast decay)
    s_shut_n = SFX_RATE * 7 / 100;
    s_shut = (s16*)linearAlloc(s_shut_n * 2);
    for (size_t i = 0; i < s_shut_n; i++){
        float t = (float)i / SFX_RATE;
        float env = expf(-t * 55.0f);
        float s = sinf(2*M_PI*2000*t) * 0.5f + ((i % 7 < 3) ? 0.35f : -0.35f);
        s_shut[i] = (s16)(s * env * 21000);
    }
    DSP_FlushDataCache(s_shut, s_shut_n * 2);
    // processing: 0.6s loop = two soft blips
    s_proc_n = SFX_RATE * 6 / 10;
    s_proc = (s16*)linearAlloc(s_proc_n * 2);
    for (size_t i = 0; i < s_proc_n; i++){
        float t = (float)i / SFX_RATE;
        float a = 0.0f;
        if (t < 0.10f)               a = sinf(2*M_PI*620*t);
        else if (t > 0.20f && t < 0.30f) a = sinf(2*M_PI*820*(t-0.20f));
        s_proc[i] = (s16)(a * 0.22f * 16000);
    }
    DSP_FlushDataCache(s_proc, s_proc_n * 2);
    return true;
}
void audio_sfx_shutter(void){
    memset(&s_shwb, 0, sizeof s_shwb);
    s_shwb.data_vaddr = s_shut; s_shwb.nsamples = s_shut_n; s_shwb.status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(1, &s_shwb);
}
void audio_proc_start(void){
    memset(&s_pwb, 0, sizeof s_pwb);
    s_pwb.data_vaddr = s_proc; s_pwb.nsamples = s_proc_n; s_pwb.looping = true; s_pwb.status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(1, &s_pwb);
}
void audio_proc_stop(void){ ndspChnWaveBufClear(1); }
void audio_play_voice(const unsigned char *pcm, size_t bytes, int rate){
    if (!pcm || bytes < 2) return;
    if (s_voice) { linearFree(s_voice); s_voice = NULL; }
    s_voice = (u8*)linearAlloc(bytes);
    if (!s_voice) return;
    memcpy(s_voice, pcm, bytes); DSP_FlushDataCache(s_voice, bytes);
    memset(&s_vwb, 0, sizeof s_vwb);
    s_vwb.data_vaddr = s_voice; s_vwb.nsamples = bytes / 2; s_vwb.status = NDSP_WBUF_FREE;
    ndspChnSetRate(0, (float)rate);
    ndspChnWaveBufAdd(0, &s_vwb);
}
bool audio_voice_playing(void){ return s_vwb.data_vaddr != NULL && s_vwb.status != NDSP_WBUF_DONE; }
void audio_exit(void){
    ndspChnWaveBufClear(0); ndspChnWaveBufClear(1);
    if (s_voice) linearFree(s_voice);
    if (s_shut) linearFree(s_shut);
    if (s_proc) linearFree(s_proc);
    ndspExit();
}
