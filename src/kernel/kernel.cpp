// src/kernel/kernel.cpp
// Флаги компиляции важны: -ffreestanding -fno-exceptions -fno-rtti

// Простые типы для удобства
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using size_t = unsigned int;

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
}

// Вывод символа на экран с учётом переносов и скроллинга
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            // Скроллинг (пока пропустим для простоты)
            terminal_row = 0;
        }
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
}

// Вывод строки
void terminal_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        terminal_putchar(str[i]);
    }
}

// Точка входа ядра
extern "C" void kernel_main(unsigned int magic) {
    terminal_clear();
    terminal_write("Nyrix Kernel v0.1\n");
    terminal_write("Copyright (c) 2026\n");
    terminal_write("Developed with passion\n");
    terminal_write("Magic number: ");

    // Простой вывод числа в hex (без snprintf)
    char hex_buf[11];
    const char hex_chars[] = "0123456789ABCDEF";
    hex_buf[0] = '0';
    hex_buf[1] = 'x';
    for (int i = 7; i >= 0; --i) {
        hex_buf[2 + 7 - i] = hex_chars[(magic >> (i * 4)) & 0xF];
    }
    hex_buf[10] = '\0';
    terminal_write(hex_buf);

    // Бесконечный цикл
    while (1) {
        __asm__ volatile("hlt");
    }
}
