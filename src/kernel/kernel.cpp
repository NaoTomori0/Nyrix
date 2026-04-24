// src/kernel/kernel.cpp
#include <stddef.h>
#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "multiboot2.h"
#include "paging.h"
#include "pit.h"
#include "pmm.h"
#include "gfx.h"
#include "task.h"

// ----- VGA-терминал (резервный, используется только если нет фреймбуфера) -----
static uint16_t *const VGA_BUFFER = reinterpret_cast<uint16_t *>(0xB8000);
static const size_t VGA_WIDTH = 80, VGA_HEIGHT = 25;
size_t terminal_row = 0, terminal_column = 0;
uint8_t terminal_color = 0x0F;
extern char input_buffer[256];
extern size_t input_pos;

static bool use_framebuffer = false;
// Глобальные параметры фреймбуфера (заполняются до paging)
uint64_t g_fb_addr = 0;
uint32_t g_fb_pitch = 0, g_fb_width = 0, g_fb_height = 0;
uint8_t g_fb_bpp = 0;
bool g_fb_found = false;

// Внутренние VGA-функции
static inline uint16_t vga_entry(unsigned char ch, uint8_t color)
{
    return static_cast<uint16_t>(ch) | (static_cast<uint16_t>(color) << 8);
}
void update_cursor(size_t row, size_t col)
{
    uint16_t pos = row * VGA_WIDTH + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}
static void vga_scroll()
{
    for (size_t y = 1; y < VGA_HEIGHT; ++y)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            VGA_BUFFER[(y - 1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
}
void terminal_clear()
{
    if (use_framebuffer)
    {
        gfx_clear();
        return;
    }
    for (size_t y = 0; y < VGA_HEIGHT; ++y)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            VGA_BUFFER[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    terminal_row = terminal_column = 0;
    update_cursor(terminal_row, terminal_column);
}
void terminal_putchar(char c)
{
    if (use_framebuffer)
    {
        gfx_putchar(c);
        return;
    }
    if (c == '\n')
    {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
        {
            vga_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        update_cursor(terminal_row, terminal_column);
        return;
    }
    if (c == '\b')
    {
        if (terminal_column > 0)
            --terminal_column;
        else if (terminal_row > 0)
        {
            --terminal_row;
            terminal_column = VGA_WIDTH - 1;
        }
        VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
        update_cursor(terminal_row, terminal_column);
        return;
    }
    VGA_BUFFER[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(c, terminal_color);
    if (++terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
        {
            vga_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
    update_cursor(terminal_row, terminal_column);
}
void terminal_write(const char *str)
{
    while (*str)
        terminal_putchar(*str++);
}
void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
    if (use_framebuffer)
    {
        gfx_set_color(0xFFFFFF, 0x000000);
    }
}
void redraw_input()
{
    if (use_framebuffer)
        return; // в графике редактор строки пока не поддерживается
    size_t prompt_len = 7;
    for (size_t i = 0; i < VGA_WIDTH; ++i)
        VGA_BUFFER[terminal_row * VGA_WIDTH + i] = vga_entry(' ', terminal_color);
    for (size_t i = 0; i < prompt_len; ++i)
        VGA_BUFFER[terminal_row * VGA_WIDTH + i] = vga_entry(("nyrix> ")[i], terminal_color);
    for (size_t i = 0; i < input_pos; ++i)
        VGA_BUFFER[terminal_row * VGA_WIDTH + prompt_len + i] = vga_entry(input_buffer[i], terminal_color);
    terminal_column = prompt_len + input_pos;
    update_cursor(terminal_row, terminal_column);
}

extern void process_command(const char *cmd);

// ----- Точка входа -----
extern "C" void kernel_main(uint32_t magic, void *mb2_info)
{
    (void)magic;

    // 1. Сбор информации (молча)
    uint64_t fb_addr = 0;
    uint32_t fb_pitch = 0, fb_width = 0, fb_height = 0;
    uint8_t fb_bpp = 0;
    bool fb_found = false;
    uint32_t mmap_addr = 0, mmap_length = 0;

    if (mb2_info)
    {
        multiboot2_tag *tag = reinterpret_cast<multiboot2_tag *>(
            reinterpret_cast<uint8_t *>(mb2_info) + 8);
        while (tag->type != 0)
        {
            if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER)
            {
                auto *fb = reinterpret_cast<multiboot2_tag_framebuffer *>(tag);
                g_fb_addr = fb->framebuffer_addr;
                g_fb_pitch = fb->framebuffer_pitch;
                g_fb_width = fb->framebuffer_width;
                g_fb_height = fb->framebuffer_height;
                g_fb_bpp = fb->framebuffer_bpp;
                g_fb_found = true;
            }
            else if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP)
            {
                auto *mtag = reinterpret_cast<multiboot2_tag_mmap *>(tag);
                mmap_addr = reinterpret_cast<uint32_t>(mtag) + sizeof(multiboot2_tag_mmap);
                mmap_length = mtag->size - sizeof(multiboot2_tag_mmap);
            }
            tag = reinterpret_cast<multiboot2_tag *>(
                reinterpret_cast<uint8_t *>(tag) + ((tag->size + 7) & ~7));
        }
    }

    // 2. Базовая инициализация без вывода
    GDT::init();
    IDT::init();
    Keyboard::init();
    __asm__ volatile("sti");

    // 3. Память и пейджинг (отладка внутри paging_init_full не видна, но и не мешает)
    if (mmap_addr)
    {
        pmm_init(mmap_addr, mmap_length);
    }
    paging_init_full();

    // Отображаем фреймбуфер (теперь page_directory доступен)
    if (g_fb_found)
    {
        uint64_t fb_size = (uint64_t)g_fb_pitch * g_fb_height;
        paging_map_framebuffer(g_fb_addr, fb_size);
    }

    // Инициализация графики
    if (g_fb_found)
    {
        gfx_init(g_fb_addr, g_fb_pitch, g_fb_width, g_fb_height, g_fb_bpp);
        use_framebuffer = true;
    }

    __asm__ volatile("outb %0, %1" : : "a"('Y'), "Nd"(0x3F8));

    if (fb_found)
    {
        gfx_init(fb_addr, fb_pitch, fb_width, fb_height, fb_bpp);
        // use_framebuffer = true;  // пока не включаем
    }

    __asm__ volatile("outb %0, %1" : : "a"('Z'), "Nd"(0x3F8));

    // 5. Теперь можно выводить сообщения
    terminal_clear();
    terminal_write("Nyrix Kernel v0.2\n");
    if (mmap_addr)
    {
        terminal_write("PMM initialized.\n");
    }
    else
    {
        terminal_write("No memory map.\n");
    }
    terminal_write("Paging enabled.\n");
    if (fb_found)
    {
        terminal_write("Framebuffer active.\n");
    }
    else
    {
        terminal_write("Using VGA fallback.\n");
    }

    terminal_write("Initializing kernel heap...\n");
    kmalloc_init();

    pit_init(100);
    tasking_init();
    terminal_write("Multitasking started.\n");

    terminal_write("Nyrix ready.\n");
    terminal_write("nyrix> ");

    while (1)
    {
        __asm__ volatile("hlt");
    }
}