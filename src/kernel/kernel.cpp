// src/kernel/kernel.cpp
#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "multiboot.h"
#include "paging.h"
#include "pmm.h"

// Адрес видеопамяти в текстовом режиме (цветной)
static uint16_t* const VGA_BUFFER = reinterpret_cast<uint16_t*>(0xB8000);
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

// Позиция курсора
size_t terminal_row = 0;
size_t terminal_column = 0;
uint8_t terminal_color = 0x0F;  // Белый на чёрном фоне

// Функция для создания 16-битного значения цвета символа
inline uint16_t vga_entry(unsigned char ch, uint8_t color) {
    return static_cast<uint16_t>(ch) | (static_cast<uint16_t>(color) << 8);
}

// Обновление аппаратного курсора
void update_cursor(size_t row, size_t col) {
    uint16_t pos = row * VGA_WIDTH + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Скроллинг: сдвигаем все строки вверх на одну
void terminal_scroll() {
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            VGA_BUFFER[(y - 1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];
        }
    }
    // Очищаем последнюю строку
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', terminal_color);
    }
}

// Очистка экрана
void terminal_clear() {
    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
    update_cursor(terminal_row, terminal_column);
}

// Вывод символа (с обработкой \n, \b и скроллингом)
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        update_cursor(terminal_row, terminal_column);
        return;
    }

    if (c == '\b') {  // Backspace
        if (terminal_column > 0) {
            --terminal_column;
        } else if (terminal_row > 0) {
            --terminal_row;
            terminal_column = VGA_WIDTH - 1;
        }
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        update_cursor(terminal_row, terminal_column);
        return;
    }

    // Обычный символ
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    VGA_BUFFER[index] = vga_entry(c, terminal_color);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
    }
    update_cursor(terminal_row, terminal_column);
}

// Вывод строки
void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        terminal_putchar(str[i]);
    }
}

// Внешняя функция обработки команд (будет в commands.cpp)
extern void process_command(const char* cmd);

// Точка входа ядра
extern "C" void kernel_main(unsigned int magic, multiboot_info* mb_info) {
    (void)magic;  // magic не используется

    GDT::init();
    IDT::init();
    Keyboard::init();
    __asm__ volatile("sti");

    terminal_clear();
    terminal_write("Nyrix Kernel v0.1\n");
    terminal_write("Copyright (c) 2026\n");
    terminal_write("Developed with passion\n");

    if (mb_info->flags & (1 << 6)) {
        terminal_write("Initializing PMM...\n");
        pmm_init(mb_info->mmap_addr, mb_info->mmap_length);
        terminal_write("PMM initialized.\n");
    } else {
        terminal_write("No memory map from GRUB, PMM disabled!\n");
    }

    kmalloc_init();
    terminal_write("Kernel heap initialized.\n");

    paging_init();
    terminal_write("Paging enabled.\n");

    terminal_write("Nyrix ready.\n");
    terminal_write("nyrix> ");

    while (1) {
        __asm__ volatile("hlt");
    }
}
