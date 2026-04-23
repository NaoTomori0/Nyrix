// src/kernel/keyboard.cpp
#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"
#include "idt.h"

extern void terminal_putchar(char c);
extern void terminal_write(const char *str);
extern void process_command(const char *cmd);

static char input_buffer[256]; // буфер для накопления строки
static size_t input_pos = 0;

static const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static bool shift_pressed = false;
static bool caps_lock = false;

void Keyboard::init() {}

void Keyboard::handle_interrupt()
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80)
    {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36)
            shift_pressed = false;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36)
    {
        shift_pressed = true;
        return;
    }
    if (scancode == 0x3A)
    {
        caps_lock = !caps_lock;
        return;
    }

    char ascii = scancode_to_ascii[scancode];
    if (ascii == 0)
        return;

    // Преобразование регистра
    if (ascii >= 'a' && ascii <= 'z')
    {
        if (shift_pressed ^ caps_lock)
            ascii -= 32;
    }
    else if (shift_pressed)
    {
        switch (ascii)
        {
        case '1':
            ascii = '!';
            break;
        case '2':
            ascii = '@';
            break;
        case '3':
            ascii = '#';
            break;
        case '4':
            ascii = '$';
            break;
        case '5':
            ascii = '%';
            break;
        case '6':
            ascii = '^';
            break;
        case '7':
            ascii = '&';
            break;
        case '8':
            ascii = '*';
            break;
        case '9':
            ascii = '(';
            break;
        case '0':
            ascii = ')';
            break;
        case '-':
            ascii = '_';
            break;
        case '=':
            ascii = '+';
            break;
        case '[':
            ascii = '{';
            break;
        case ']':
            ascii = '}';
            break;
        case '\\':
            ascii = '|';
            break;
        case ';':
            ascii = ':';
            break;
        case '\'':
            ascii = '"';
            break;
        case ',':
            ascii = '<';
            break;
        case '.':
            ascii = '>';
            break;
        case '/':
            ascii = '?';
            break;
        case '`':
            ascii = '~';
            break;
        }
    }

    if (ascii == '\n')
    {
        input_buffer[input_pos] = '\0';
        terminal_putchar('\n');
        process_command(input_buffer);
        input_pos = 0;
        terminal_write("nyrix> ");
        return;
    }

    if (ascii == '\b')
    {
        if (input_pos > 0)
        {
            input_pos--;
            terminal_putchar('\b');
        }
        return;
    }

    // Обычные символы
    if (input_pos < sizeof(input_buffer) - 1)
    {
        input_buffer[input_pos++] = ascii;
        terminal_putchar(ascii);
    }
}