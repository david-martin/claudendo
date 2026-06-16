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

// Copy a CAM_W x CAM_H RGB565 image to the top-screen framebuffer (handles the 90deg rotation).
static void draw_top(const u16 *img){
    u16 *fb = (u16*)(void*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    for (int x = 0; x < CAM_W; x++)
        for (int y = 0; y < CAM_H; y++)
            fb[x*CAM_H + (CAM_H-1-y)] = img[y*CAM_W + x];
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_TOP, false);
}

static void show_bottom(const char *host, const char *status, const char *desc){
    consoleClear();
    printf("claudendo\nhost: %s\n\n", host);
    if (status && *status) printf("%s\n\n", status);
    if (desc && *desc)     printf("> %s\n\n", desc);
    printf("\x1b[29;0HA: describe   START: exit");
}

int main(void){
    gfxInitDefault();
    gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
    PrintConsole bottom;
    consoleInit(GFX_BOTTOM, &bottom);

    u32 *soc = (u32*)memalign(0x1000, SOC_BUF);
    bool net = soc && R_SUCCEEDED(socInit(soc, SOC_BUF));
    bool ps  = R_SUCCEEDED(psInit());
    bool rom = R_SUCCEEDED(romfsInit());
    bool aud = audio_init();
    bool cam = camera_init();
    Config cfg;
    bool ready = net && ps && rom && config_load(&cfg);

    if (!ready) {
        consoleClear();
        if (!net||!ps||!rom) printf("init failed (net/ps/romfs)\n");
        else printf("Missing sdmc:/claudendo/config.cfg\n(line1=host, line2=token)\n");
        printf("\nPress START to exit\n");
        while (aptMainLoop()){ hidScanInput(); if (hidKeysDown()&KEY_START) break; gspWaitForVBlank(); }
    } else {
        u16 *preview = (u16*)linearAlloc(CAM_W*CAM_H*2);
        u16 *frozen  = (u16*)linearAlloc(CAM_W*CAM_H*2);
        show_bottom(cfg.host, aud ? "Live. Point and press A." : "Live (no audio). Press A.", NULL);
        while (aptMainLoop()){
            hidScanInput();
            u32 k = hidKeysDown();
            if (k & KEY_START) break;

            if (cam && camera_frame(preview)) draw_top(preview);

            if ((k & KEY_A) && cam) {
                memcpy(frozen, preview, CAM_W*CAM_H*2);
                draw_top(frozen);
                if (aud) audio_sfx_shutter();
                show_bottom(cfg.host, "Thinking...", NULL);
                if (aud) audio_proc_start();
                char desc[600]={0}; u8 *pcm=NULL; size_t plen=0; int rate=22050;
                int st = http_describe(cfg.host, cfg.token, (u8*)frozen, CAM_W, CAM_H,
                                       desc, sizeof desc, &pcm, &plen, &rate);
                if (aud) audio_proc_stop();
                if (st == 200)      show_bottom(cfg.host, NULL, desc);
                else if (st < 0)    show_bottom(cfg.host, "connection failed", desc[0]?desc:NULL);
                else { char e[64]; snprintf(e,sizeof e,"error %d", st); show_bottom(cfg.host, e, desc[0]?desc:NULL); }
                if (pcm && plen && aud) {
                    audio_play_voice(pcm, plen, rate);
                    while (audio_voice_playing() && aptMainLoop()){
                        hidScanInput(); if (hidKeysDown()&KEY_START) goto done;
                        draw_top(frozen); gspWaitForVBlank();
                    }
                }
                if (pcm) linearFree(pcm);
                show_bottom(cfg.host, "Live. Point and press A.", desc[0]?desc:NULL);
            }
            gspWaitForVBlank();
        }
        done:
        if (preview) linearFree(preview);
        if (frozen)  linearFree(frozen);
    }
    if (cam) camera_exit();
    if (aud) audio_exit();
    gfxExit(); return 0;
}
