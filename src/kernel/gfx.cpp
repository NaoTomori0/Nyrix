// src/kernel/gfx.cpp
#include "gfx.h"
#include <stdint.h>
#include <stddef.h>

static uint8_t* framebuffer;
static uint32_t fb_pitch, fb_width, fb_height;
static uint8_t  fb_bpp;

static uint32_t fg_color = 0x00FFFFFF;
static uint32_t bg_color = 0x00000000;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
const uint32_t CHAR_W = 8;
const uint32_t CHAR_H = 16;

// Шрифт (оставляем без изменений)
static const uint8_t font[95][16] = {
    // ... ваш код шрифта (приводить не буду, оставьте как есть)
};

static const uint8_t* get_font_char(char ch) {
    if (ch < 32 || ch > 126) return font[0];
    return font[ch - 32];
}

void gfx_init(uint64_t fb_addr, uint32_t pitch, uint32_t width, uint32_t height, uint8_t bpp) {
    framebuffer = reinterpret_cast<uint8_t*>(fb_addr);
    fb_pitch = pitch;
    fb_width  = width;
    fb_height = height;
    fb_bpp    = bpp;
    for (uint32_t y = 0; y < height; ++y) {
        uint32_t* row = reinterpret_cast<uint32_t*>(framebuffer + y * pitch);
        for (uint32_t x = 0; x < width; ++x) {
            row[x] = bg_color;
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void gfx_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += CHAR_H;
        if (cursor_y + CHAR_H >= fb_height) gfx_scroll();
        return;
    }
    if (c == '\b') {
        if (cursor_x >= CHAR_W) cursor_x -= CHAR_W;
        else if (cursor_y >= CHAR_H) { cursor_y -= CHAR_H; cursor_x = fb_width - CHAR_W; }
        // Заливка фона на позиции Backspace
        for (uint32_t y = 0; y < CHAR_H; ++y) {
            uint32_t* row = (uint32_t*)(framebuffer + (cursor_y + y) * fb_pitch + cursor_x * (fb_bpp/8));
            for (uint32_t x = 0; x < CHAR_W; ++x) row[x] = bg_color;
        }
        return;
    }

    const uint8_t* char_bitmap = get_font_char(c);
    for (uint32_t y = 0; y < CHAR_H; ++y) {
        uint32_t* row = (uint32_t*)(framebuffer + (cursor_y + y) * fb_pitch + cursor_x * (fb_bpp/8));
        uint8_t bits = char_bitmap[y];
        for (uint32_t x = 0; x < CHAR_W; ++x) {
            row[x] = (bits & (0x80 >> x)) ? fg_color : bg_color;
        }
    }

    cursor_x += CHAR_W;
    if (cursor_x + CHAR_W > fb_width) {
        cursor_x = 0;
        cursor_y += CHAR_H;
        if (cursor_y + CHAR_H >= fb_height) gfx_scroll();
    }
}

void gfx_scroll() {
    uint32_t row_size = fb_pitch * CHAR_H;
    for (uint32_t y = 0; y < fb_height - CHAR_H; ++y) {
        uint8_t* dst = framebuffer + y * fb_pitch;
        uint8_t* src = framebuffer + (y + CHAR_H) * fb_pitch;
        for (uint32_t x = 0; x < fb_width * (fb_bpp/8); ++x) dst[x] = src[x];
    }
    for (uint32_t y = fb_height - CHAR_H; y < fb_height; ++y) {
        uint32_t* row = (uint32_t*)(framebuffer + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; ++x) row[x] = bg_color;
    }
}

void gfx_clear() {
    for (uint32_t y = 0; y < fb_height; ++y) {
        uint32_t* row = (uint32_t*)(framebuffer + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; ++x) row[x] = bg_color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void gfx_set_color(uint32_t fg, uint32_t bg) {
    fg_color = fg;
    bg_color = bg;
}