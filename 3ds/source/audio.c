#include "audio.h"
#include <string.h>
#include <malloc.h>
static u8 *s_buf = NULL;
static ndspWaveBuf s_wb;
bool audio_init(void){
    if (R_FAILED(ndspInit())) return false;     // needs DSP firmware (present on homebrew CFW)
    ndspSetOutputMode(NDSP_OUTPUT_MONO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    return true;
}
void audio_play(const unsigned char *pcm, size_t bytes, int rate){
    if (!pcm || bytes < 2) return;
    if (s_buf) { linearFree(s_buf); s_buf = NULL; }
    s_buf = (u8*)linearAlloc(bytes);
    if (!s_buf) return;
    memcpy(s_buf, pcm, bytes);
    DSP_FlushDataCache(s_buf, bytes);
    memset(&s_wb, 0, sizeof s_wb);
    s_wb.data_vaddr = s_buf;
    s_wb.nsamples   = bytes / 2;       // PCM16 mono
    s_wb.status     = NDSP_WBUF_FREE;
    ndspChnSetRate(0, (float)rate);
    ndspChnWaveBufAdd(0, &s_wb);
    while (s_wb.status != NDSP_WBUF_DONE && aptMainLoop()) gspWaitForVBlank();
}
void audio_exit(void){ if (s_buf) linearFree(s_buf); ndspExit(); }
