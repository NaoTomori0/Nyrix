#ifndef NYRIX_GFX_H
#define NYRIX_GFX_H

#include <stdint.h>

void gfx_init(uint64_t fb_addr, uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp);
void gfx_putchar(char c);
void gfx_clear();
void gfx_set_color(uint32_t fg, uint32_t bg);
void gfx_scroll();
#endif