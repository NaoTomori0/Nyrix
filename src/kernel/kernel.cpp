// src/kernel/kernel.cpp
// Флаги компиляции важны: -ffreestanding -fno-exceptions -fno-rtti
#include <stdint.h>
#include <stddef.h>
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"

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

// Вывод символа на экран с учётом переносов и скроллинга
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        update_cursor(terminal_row, terminal_column);
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    VGA_BUFFER[index] = vga_entry(c, terminal_color);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
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

// Точка входа ядра
extern "C" void kernel_main(unsigned int magic) {
    GDT::init();
    IDT::init();
    Keyboard::init();
    __asm__ volatile("sti");  // разрешаем аппаратные прерывания

    terminal_clear();
    terminal_write("Nyrix Kernel v0.1\n");
    terminal_write("Copyright (c) 2026\n");
    terminal_write("Developed with passion\n");
    terminal_write("Magic number: ");

    char hex_buf[11];
    const char hex_chars[] = "0123456789ABCDEF";
    hex_buf[0] = '0';
    hex_buf[1] = 'x';
    for (int i = 7; i >= 0; --i) {
        hex_buf[2 + 7 - i] = hex_chars[(magic >> (i * 4)) & 0xF];
    }
    hex_buf[10] = '\0';
    terminal_write(hex_buf);
    terminal_putchar('\n');

    // Бесконечный цикл ожидания
    while (1) {
        __asm__ volatile("hlt");
    }
}