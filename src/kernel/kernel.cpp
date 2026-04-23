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
#include "task.h"

// ----- VGA-терминал -----
static uint16_t *const VGA_BUFFER = reinterpret_cast<uint16_t *>(0xB8000);
static const size_t VGA_WIDTH = 80, VGA_HEIGHT = 25;
size_t terminal_row = 0, terminal_column = 0; // глобальные переменные
uint8_t terminal_color = 0x0F;

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

void terminal_setcolor(uint8_t new_color)
{
    terminal_color = new_color;
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
    for (size_t y = 0; y < VGA_HEIGHT; ++y)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            VGA_BUFFER[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    terminal_row = terminal_column = 0;
    update_cursor(terminal_row, terminal_column);
}

void terminal_putchar(char c)
{
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

extern void process_command(const char *cmd);

// ----- Буфер ввода и редактор (для keyboard) -----
extern char input_buffer[256];
extern size_t input_pos;

void redraw_input()
{
    size_t prompt_len = 7; // "nyrix> "
    // Очищаем текущую строку
    for (size_t i = 0; i < VGA_WIDTH; ++i)
    {
        VGA_BUFFER[terminal_row * VGA_WIDTH + i] = vga_entry(' ', terminal_color);
    }
    // Выводим приглашение и содержимое буфера
    for (size_t i = 0; i < prompt_len; ++i)
    {
        VGA_BUFFER[terminal_row * VGA_WIDTH + i] = vga_entry(("nyrix> ")[i], terminal_color);
    }
    for (size_t i = 0; i < input_pos; ++i)
    {
        VGA_BUFFER[terminal_row * VGA_WIDTH + prompt_len + i] = vga_entry(input_buffer[i], terminal_color);
    }
    terminal_column = prompt_len + input_pos;
    update_cursor(terminal_row, terminal_column);
}

// ----- Точка входа -----
extern "C" void kernel_main(uint32_t magic, void *mb2_info)
{
    (void)magic;

    uint32_t mmap_addr = 0, mmap_length = 0;
    if (mb2_info)
    {
        multiboot2_tag *tag = reinterpret_cast<multiboot2_tag *>(
            reinterpret_cast<uint8_t *>(mb2_info) + 8);
        while (tag->type != 0)
        {
            if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP)
            {
                auto *mtag = reinterpret_cast<multiboot2_tag_mmap *>(tag);
                mmap_addr = reinterpret_cast<uint32_t>(mtag) + sizeof(multiboot2_tag_mmap);
                mmap_length = mtag->size - sizeof(multiboot2_tag_mmap);
                break;
            }
            tag = reinterpret_cast<multiboot2_tag *>(
                reinterpret_cast<uint8_t *>(tag) + ((tag->size + 7) & ~7));
        }
    }

    GDT::init();
    IDT::init();
    Keyboard::init();
    __asm__ volatile("sti");

    terminal_clear();
    terminal_write("Nyrix Kernel v0.2\n");

    if (mmap_addr)
    {
        terminal_write("Initializing PMM...\n");
        pmm_init(mmap_addr, mmap_length);
        // terminal_write("PMM works!\n");
    }
    else
    {
        terminal_write("No memory map, using limited PMM.\n");
    }

    terminal_write("Enabling paging (4MB)...\n");
    paging_init_simple();
    // terminal_write("Paging enabled.\n");

    terminal_write("Initializing kernel heap...\n");
    kmalloc_init();
    // terminal_write("Kernel heap ready.\n");

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