#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "config.h"
#include "camera.h"
#include "audio.h"
#include "http.h"
#include "gfx2d.h"
#define SOC_BUF (0x100000)

// ---- personas -------------------------------------------------------------
#define N_PERSONA 4
static const char *PERSONA_KEY[N_PERSONA]  = {"marvin", "bobross", "attenborough", "mayaangelou"};
static const char *PERSONA_NAME[N_PERSONA] = {"Marvin", "Bob Ross", "Attenborough", "Maya Angelou"};
static const char *PERSONA_TAG[N_PERSONA]  = {
    "Brain the size of a planet...",
    "Happy little accidents.",
    "Remarkable, isn't it?",
    "Still, I rise.",
};
static u16 ACCENT[N_PERSONA];   // filled in main() (macro needs runtime use is fine, but keep simple)

// ---- palette --------------------------------------------------------------
#define COL_BG       GFX_RGB565(14,16,22)
#define COL_PANEL    GFX_RGB565(26,30,40)
#define COL_TEXT     GFX_RGB565(236,238,240)
#define COL_DIM      GFX_RGB565(120,132,146)
#define COL_HEADERTX GFX_RGB565(12,14,18)
#define COL_WHITE    GFX_RGB565(255,255,255)

// ---- bottom-screen layout (320x240) --------------------------------------
#define CARD_X 8
#define CARD_W 304
#define CARD_Y0 34
#define CARD_H 28
#define BTN_Y 150
#define BTN_H 30
#define CAMBTN_X 8
#define CAMBTN_W 148
#define DESCBTN_X 164
#define DESCBTN_W 148

// hit-test bottom-screen touch -> action id
// 0..N_PERSONA-1 = pick persona, 100 = toggle camera, 101 = describe, -1 = none
static int hit_test(int x, int y){
    for (int i = 0; i < N_PERSONA; i++){
        int cy = CARD_Y0 + i*CARD_H;
        if (x >= CARD_X && x < CARD_X+CARD_W && y >= cy && y < cy+CARD_H) return i;
    }
    if (x >= CAMBTN_X  && x < CAMBTN_X+CAMBTN_W   && y >= BTN_Y && y < BTN_Y+BTN_H) return 100;
    if (x >= DESCBTN_X && x < DESCBTN_X+DESCBTN_W && y >= BTN_Y && y < BTN_Y+BTN_H) return 101;
    return -1;
}

static GfxTarget top_target(void){
    GfxTarget t; t.fb = (u16*)(void*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL); t.w = 400; t.h = 240;
    return t;
}
static GfxTarget bot_target(void){
    GfxTarget t; t.fb = (u16*)(void*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL); t.w = 320; t.h = 240;
    return t;
}

// Blit the camera frame to the top framebuffer (handles 90deg rotation; mirrors for selfie).
static void blit_camera(u16 *fb, const u16 *img, bool mirror){
    for (int x = 0; x < CAM_W; x++)
        for (int y = 0; y < CAM_H; y++){
            int sx = mirror ? (CAM_W-1-x) : x;
            fb[x*CAM_H + (CAM_H-1-y)] = img[y*CAM_W + sx];
        }
}

// Viewfinder corner brackets + persona-coloured border.
static void top_overlay(const GfxTarget *t, int persona, bool inner){
    u16 a = ACCENT[persona];
    gfx_border(t, 0, 0, t->w, t->h, 3, a);
    int L = 26, th = 3, m = 10;
    // four corner brackets in white
    for (int c = 0; c < 4; c++){
        int cx = (c & 1) ? t->w - m - L : m;
        int cy = (c & 2) ? t->h - m - L : m;
        gfx_fill_rect(t, cx, (c & 2) ? cy + L - th : cy, L, th, COL_WHITE);   // horizontal
        gfx_fill_rect(t, (c & 1) ? cx + L - th : cx, cy, th, L, COL_WHITE);   // vertical
    }
    // labels: persona top-left, camera top-right
    gfx_text_bg(t, m + 6, m + 4, 2, a, COL_HEADERTX, PERSONA_NAME[persona]);
    const char *cam = inner ? "INNER" : "OUTER";
    gfx_text_bg(t, t->w - m - 6 - gfx_text_width(1, cam), m + 6, 1, COL_TEXT, COL_HEADERTX, cam);
}

// Frozen photo with optional dim + optional subtitle box.
static void draw_top_frozen(const u16 *frozen, int persona, bool inner, bool dim, const char *subtitle){
    GfxTarget t = top_target();
    blit_camera(t.fb, frozen, inner);
    if (dim){ gfx_darken_rect(&t, 0, 0, t.w, t.h); gfx_darken_rect(&t, 0, 0, t.w, t.h); }
    gfx_border(&t, 0, 0, t.w, t.h, 3, ACCENT[persona]);
    if (dim){
        const char *msg = "thinking...";
        gfx_text(&t, (t.w - gfx_text_width(3, msg))/2, t.h/2 - 12, 3, COL_WHITE, msg);
    }
    if (subtitle && *subtitle){
        int boxh = 96, by = t.h - boxh;   // bottom 96px; 1x text wraps to fit 2 sentences
        gfx_darken_rect(&t, 0, by, t.w, boxh);
        gfx_darken_rect(&t, 0, by, t.w, boxh);
        gfx_fill_rect(&t, 0, by, t.w, 2, ACCENT[persona]);
        gfx_text_wrap(&t, 12, by + 8, t.w - 24, 1, 5, COL_WHITE, subtitle);
    }
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_TOP, false);
}

static void flash_top(const u16 *frozen, int persona, bool inner){
    GfxTarget t = top_target();
    gfx_clear(&t, COL_WHITE);
    gfxFlushBuffers(); gfxScreenSwapBuffers(GFX_TOP, false);
    gspWaitForVBlank(); gspWaitForVBlank();
    // restore frozen on the other buffer
    draw_top_frozen(frozen, persona, inner, false, NULL);
}

static void draw_button(const GfxTarget *t, int x, int y, int w, int h,
                        const char *label, const char *hint, u16 fg, u16 bg, u16 border){
    gfx_fill_rect(t, x, y, w, h, bg);
    gfx_border(t, x, y, w, h, 1, border);
    int tw = gfx_text_width(2, label);
    gfx_text(t, x + (w - tw)/2, y + 4, 2, fg, label);
    if (hint && *hint){
        int hw = gfx_text_width(1, hint);
        gfx_text(t, x + (w - hw)/2, y + 21, 1, fg, hint);
    }
}

static void draw_bottom(int persona, bool inner, const char *status){
    GfxTarget t = bot_target();
    u16 a = ACCENT[persona];
    gfx_clear(&t, COL_BG);

    // header bar (persona accent). Tagline lives on the cards, not here, so it
    // never collides with the title.
    gfx_fill_rect(&t, 0, 0, t.w, 28, a);
    gfx_text(&t, 8, 6, 2, COL_HEADERTX, "claudendo");

    // persona cards
    for (int i = 0; i < N_PERSONA; i++){
        int cy = CARD_Y0 + i*CARD_H;
        bool sel = (i == persona);
        if (sel){
            gfx_fill_rect(&t, CARD_X, cy, CARD_W, CARD_H-2, ACCENT[i]);
            gfx_darken_rect(&t, CARD_X, cy, CARD_W, CARD_H-2);
            gfx_darken_rect(&t, CARD_X, cy, CARD_W, CARD_H-2);
            gfx_border(&t, CARD_X, cy, CARD_W, CARD_H-2, 1, ACCENT[i]);
        } else {
            gfx_fill_rect(&t, CARD_X, cy, CARD_W, CARD_H-2, COL_PANEL);
        }
        gfx_fill_rect(&t, CARD_X, cy, 4, CARD_H-2, ACCENT[i]);          // accent stripe
        if (sel) gfx_text(&t, CARD_X + 10, cy + 3, 2, ACCENT[i], ">");
        gfx_text(&t, CARD_X + 28, cy + 2,  2, sel ? COL_TEXT : COL_DIM, PERSONA_NAME[i]);
        gfx_text(&t, CARD_X + 28, cy + 18, 1, sel ? ACCENT[i] : COL_DIM, PERSONA_TAG[i]);
    }

    // camera + describe buttons
    char camlbl[24]; snprintf(camlbl, sizeof camlbl, "CAM:%s", inner ? "Inner" : "Outer");
    draw_button(&t, CAMBTN_X, BTN_Y, CAMBTN_W, BTN_H, camlbl, "(L/R)", COL_TEXT, COL_PANEL, COL_DIM);
    draw_button(&t, DESCBTN_X, BTN_Y, DESCBTN_W, BTN_H, "DESCRIBE", "(A)", COL_HEADERTX, a, a);

    // status strip
    if (status && *status){
        gfx_fill_rect(&t, 8, 188, 304, 22, COL_PANEL);
        gfx_text(&t, 14, 194, 1, COL_TEXT, status);
    }

    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_BOTTOM, false);
}

int main(void){
    gfxInitDefault();
    gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB565_OES);
    // Both screens stay double-buffered (default). Disabling DB on the bottom left
    // its displayed buffer in the default 24-bit format -> RGB565 writes garbled it.
    // We redraw + swap both screens every frame instead.

    ACCENT[0] = GFX_RGB565(120,150,175);   // Marvin   - steel blue
    ACCENT[1] = GFX_RGB565(90,190,95);     // Bob Ross - warm green
    ACCENT[2] = GFX_RGB565(225,175,70);    // Attenb.  - amber
    ACCENT[3] = GFX_RGB565(185,120,215);   // Maya     - violet

    u32 *soc = (u32*)memalign(0x1000, SOC_BUF);
    bool net = soc && R_SUCCEEDED(socInit(soc, SOC_BUF));
    bool ps  = R_SUCCEEDED(psInit());
    bool rom = R_SUCCEEDED(romfsInit());
    bool aud = audio_init();
    bool cam = camera_init();
    Config cfg;
    bool ready = net && ps && rom && config_load(&cfg);

    if (!ready) {
        while (aptMainLoop()){
            GfxTarget t = bot_target();
            gfx_clear(&t, COL_BG);
            gfx_text(&t, 10, 16, 2, GFX_RGB565(230,90,90), "claudendo - setup");
            if (!net||!ps||!rom) gfx_text_wrap(&t, 10, 56, 300, 1, 4, COL_TEXT, "init failed (net/ps/romfs).");
            else gfx_text_wrap(&t, 10, 56, 300, 1, 4, COL_TEXT,
                "Missing sdmc:/claudendo/config.cfg  (line 1 = host, line 2 = token).");
            gfx_text(&t, 10, 210, 1, COL_DIM, "Press START to exit");
            gfxFlushBuffers(); gfxScreenSwapBuffers(GFX_BOTTOM, false);
            hidScanInput(); if (hidKeysDown()&KEY_START) break; gspWaitForVBlank();
        }
    } else {
        u16 *preview = (u16*)linearAlloc(CAM_W*CAM_H*2);
        u16 *frozen  = (u16*)linearAlloc(CAM_W*CAM_H*2);
        int persona = 0;
        bool inner = false;
        char last_desc[600] = {0};
        const char *READY = aud ? "Ready. Frame it and shoot." : "Ready (no audio).";
        const char *status = READY;
        draw_bottom(persona, inner, status);

        while (aptMainLoop()){
            hidScanInput();
            u32 k = hidKeysDown();
            if (k & KEY_START) break;

            bool shoot = (k & KEY_A);

            if (k & KEY_DUP)   { persona = (persona + N_PERSONA - 1) % N_PERSONA; }
            if (k & KEY_DDOWN) { persona = (persona + 1) % N_PERSONA; }
            if (k & (KEY_L | KEY_R)) { inner = !inner; if (cam) camera_set_inner(inner); }
            if (k & KEY_TOUCH) {
                touchPosition tp; hidTouchRead(&tp);
                int act = hit_test(tp.px, tp.py);
                if (act >= 0 && act < N_PERSONA) { persona = act; }
                else if (act == 100) { inner = !inner; if (cam) camera_set_inner(inner); }
                else if (act == 101) { shoot = true; }
            }
            status = READY;
            draw_bottom(persona, inner, status);   // double-buffered: redraw every frame

            // live preview
            if (cam && camera_frame(preview)) {
                GfxTarget t = top_target();
                blit_camera(t.fb, preview, inner);
                top_overlay(&t, persona, inner);
                gfxFlushBuffers();
                gfxScreenSwapBuffers(GFX_TOP, false);
            }

            if (shoot && cam) {
                memcpy(frozen, preview, CAM_W*CAM_H*2);
                draw_top_frozen(frozen, persona, inner, false, NULL);
                if (aud) audio_sfx_shutter();
                flash_top(frozen, persona, inner);
                draw_top_frozen(frozen, persona, inner, true, NULL);   // dim + "thinking..."
                status = "Thinking..."; draw_bottom(persona, inner, status);
                if (aud) audio_proc_start();
                char desc[600]={0}; u8 *pcm=NULL; size_t plen=0; int rate=22050;
                int st = http_describe(cfg.host, cfg.token, PERSONA_KEY[persona],
                                       (u8*)frozen, CAM_W, CAM_H,
                                       desc, sizeof desc, &pcm, &plen, &rate);
                if (aud) audio_proc_stop();
                if (st == 200) { strncpy(last_desc, desc, sizeof last_desc - 1); last_desc[sizeof last_desc - 1] = 0; status = "Speaking..."; }
                else if (st < 0) { snprintf(desc, sizeof desc, "connection failed"); status = "Connection failed"; }
                else { snprintf(desc, sizeof desc, "relay error %d", st); status = "Relay error"; }
                draw_bottom(persona, inner, status);

                const char *sub = (st == 200 && last_desc[0]) ? last_desc : desc;
                draw_top_frozen(frozen, persona, inner, false, sub);
                if (pcm && plen && aud) {
                    audio_play_voice(pcm, plen, rate);
                    while (audio_voice_playing() && aptMainLoop()){
                        hidScanInput(); if (hidKeysDown()&KEY_START) goto done;
                        draw_top_frozen(frozen, persona, inner, false, sub);
                        gspWaitForVBlank();
                    }
                } else {
                    for (int i = 0; i < 210 && aptMainLoop(); i++){
                        hidScanInput(); if (hidKeysDown()&KEY_START) goto done;
                        draw_top_frozen(frozen, persona, inner, false, sub);
                        gspWaitForVBlank();
                    }
                }
                if (pcm) linearFree(pcm);
                status = READY; draw_bottom(persona, inner, status);
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
