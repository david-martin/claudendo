#pragma once
#include <3ds.h>

#define GFX_RGB565(r,g,b) ((u16)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|(((b)&0xF8)>>3)))

// An RGB565 3DS framebuffer plus its logical dimensions.
typedef struct { u16 *fb; int w; int h; } GfxTarget;

void gfx_clear(const GfxTarget *t, u16 color);
void gfx_pixel(const GfxTarget *t, int x, int y, u16 color);
void gfx_fill_rect(const GfxTarget *t, int x, int y, int w, int h, u16 color);
void gfx_border(const GfxTarget *t, int x, int y, int w, int h, int thick, u16 color);
void gfx_darken_rect(const GfxTarget *t, int x, int y, int w, int h); // ~halve brightness of region
void gfx_text(const GfxTarget *t, int x, int y, int scale, u16 fg, const char *s);        // transparent bg
void gfx_text_bg(const GfxTarget *t, int x, int y, int scale, u16 fg, u16 bg, const char *s);
int  gfx_text_width(int scale, const char *s);   // pixel width (n chars * 8 * scale)
int  gfx_text_height(int scale);                 // 8*scale
// word-wrap s within boxw pixels, drawing successive lines starting at (x,y),
// advancing by (gfx_text_height(scale)+line_gap). returns number of lines drawn.
int  gfx_text_wrap(const GfxTarget *t, int x, int y, int boxw, int scale, int line_gap, u16 fg, const char *s);
