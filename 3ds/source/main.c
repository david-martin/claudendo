#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "config.h"
#include "camera.h"
#include "audio.h"
#include "http.h"
#define SOC_BUF (0x100000)
int main(void){
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    printf("claudendo\n\n");
    u32 *soc = (u32*)memalign(0x1000, SOC_BUF);
    bool net = soc && R_SUCCEEDED(socInit(soc, SOC_BUF));
    bool ps  = R_SUCCEEDED(psInit());
    bool rom = R_SUCCEEDED(romfsInit());
    bool aud = audio_init();
    bool cam = camera_init();
    Config cfg;
    if (!net || !ps || !rom) { printf("init failed (net/ps/romfs)\n"); }
    else if (!config_load(&cfg)) { printf("Missing sdmc:/claudendo/config.cfg\n(line1=host, line2=token)\n"); }
    else {
        printf("host: %s\n", cfg.host);
        if (!aud) printf("(audio init failed - no sound)\n");
        if (!cam) printf("(camera init failed)\n");
        printf("\nPress A to describe, START to exit\n");
        u16 *img = (u16*)linearAlloc(CAM_W*CAM_H*2);
        while (aptMainLoop()) {
            hidScanInput();
            u32 k = hidKeysDown();
            if (k & KEY_START) break;
            if ((k & KEY_A) && cam && img) {
                printf("\nCapturing...\n");
                if (!camera_capture(img)) { printf("capture failed\n"); }
                else {
                    printf("Thinking...\n");
                    char desc[512]={0}; u8 *pcm=NULL; size_t plen=0; int rate=22050;
                    int st = http_describe(cfg.host, cfg.token, (u8*)img, CAM_W, CAM_H,
                                           desc, sizeof desc, &pcm, &plen, &rate);
                    if (st == 200) { printf("> %s\n", desc); }
                    else if (st < 0) { printf("connection failed (%d)\n", st); }
                    else { printf("error %d: %s\n", st, desc); }
                    if (pcm && plen && aud) audio_play(pcm, plen, rate);
                    if (pcm) linearFree(pcm);
                    printf("\nPress A again, START to exit\n");
                }
            }
            gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
        }
        if (img) linearFree(img);
    }
    if (cam) camera_exit();
    if (aud) audio_exit();
    printf("\n(exiting) Press START\n");
    while (aptMainLoop()){ hidScanInput(); if (hidKeysDown()&KEY_START) break; gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank(); }
    gfxExit(); return 0;
}
