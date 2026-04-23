// src/kernel/keyboard.cpp
#include <stdint.h>
#include <stddef.h>
#include "keyboard.h"
#include "idt.h"

extern void terminal_putchar(char c);
extern void terminal_write(const char *str);
extern void process_command(const char *cmd);
extern void redraw_input(); // из kernel.cpp

// Глобальные буфер и позиция (используются в kernel.cpp для redraw)
char input_buffer[256];
size_t input_pos = 0;

// История команд
#define MAX_HISTORY 10
static char history[MAX_HISTORY][256];
static int history_count = 0;
static int history_index = -1;
static char saved_input[256];

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
static bool extended = false;

void Keyboard::init() {}

void Keyboard::handle_interrupt()
{
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0)
    {
        extended = true;
        return;
    }

    if (scancode & 0x80)
    {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36)
            shift_pressed = false;
        extended = false;
        return;
    }

    if (extended)
    {
        switch (scancode)
        {
        case 0x48: // стрелка вверх
            if (history_count > 0)
            {
                if (history_index == -1)
                {
                    for (size_t i = 0; i < input_pos; ++i)
                        saved_input[i] = input_buffer[i];
                    saved_input[input_pos] = '\0';
                    history_index = history_count - 1;
                }
                else if (history_index > 0)
                {
                    history_index--;
                }
                const char *cmd = history[history_index];
                size_t len = 0;
                while (cmd[len])
                    len++;
                for (size_t i = 0; i < len; ++i)
                    input_buffer[i] = cmd[i];
                input_pos = len;
                redraw_input();
            }
            extended = false;
            return;
        case 0x50: // стрелка вниз
            if (history_index != -1)
            {
                if (history_index < history_count - 1)
                {
                    history_index++;
                    const char *cmd = history[history_index];
                    size_t len = 0;
                    while (cmd[len])
                        len++;
                    for (size_t i = 0; i < len; ++i)
                        input_buffer[i] = cmd[i];
                    input_pos = len;
                }
                else
                {
                    history_index = -1;
                    size_t len = 0;
                    while (saved_input[len])
                        len++;
                    for (size_t i = 0; i < len; ++i)
                        input_buffer[i] = saved_input[i];
                    input_pos = len;
                }
                redraw_input();
            }
            extended = false;
            return;
        case 0x4B: // стрелка влево
            if (input_pos > 0)
                input_pos--;
            redraw_input();
            extended = false;
            return;
        case 0x4D: // стрелка вправо
            if (input_pos < sizeof(input_buffer) - 1 && input_buffer[input_pos] != '\0')
                input_pos++;
            redraw_input();
            extended = false;
            return;
        case 0x47: // Home
            input_pos = 0;
            redraw_input();
            extended = false;
            return;
        case 0x4F: // End
            while (input_buffer[input_pos] != '\0' && input_pos < sizeof(input_buffer) - 1)
                input_pos++;
            redraw_input();
            extended = false;
            return;
        case 0x53: // Delete
            if (input_buffer[input_pos] != '\0')
            {
                size_t i = input_pos;
                while (input_buffer[i])
                {
                    input_buffer[i] = input_buffer[i + 1];
                    i++;
                }
                redraw_input();
            }
            extended = false;
            return;
        default:
            extended = false;
            return;
        }
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
        if (input_pos > 0)
        {
            process_command(input_buffer);

            // Добавляем в историю только если она не совпадает с последней командой
            bool duplicate = false;
            if (history_count > 0)
            {
                const char *last_cmd = history[history_count - 1];
                size_t i = 0;
                while (input_buffer[i] && last_cmd[i] && input_buffer[i] == last_cmd[i])
                    i++;
                if (input_buffer[i] == '\0' && last_cmd[i] == '\0')
                {
                    duplicate = true;
                }
            }

            if (!duplicate)
            {
                if (history_count < MAX_HISTORY)
                {
                    int idx = history_count;
                    history_count++;
                    size_t i = 0;
                    while (input_buffer[i])
                    {
                        history[idx][i] = input_buffer[i];
                        i++;
                    }
                    history[idx][i] = '\0';
                }
                else
                {
                    // Сдвигаем историю на одну вниз
                    for (int i = 1; i < MAX_HISTORY; ++i)
                    {
                        for (size_t j = 0; j < 256; ++j)
                            history[i - 1][j] = history[i][j];
                    }
                    size_t i = 0;
                    while (input_buffer[i])
                    {
                        history[MAX_HISTORY - 1][i] = input_buffer[i];
                        i++;
                    }
                    history[MAX_HISTORY - 1][i] = '\0';
                }
            }
        }
        input_pos = 0;
        history_index = -1;
        terminal_write("nyrix> ");
        return;
    }

    if (ascii == '\b')
    {
        if (input_pos > 0)
        {
            for (size_t i = input_pos - 1; i < sizeof(input_buffer) - 1; ++i)
            {
                input_buffer[i] = input_buffer[i + 1];
                if (input_buffer[i] == '\0')
                    break;
            }
            input_pos--;
            redraw_input();
        }
        return;
    }

    if (input_pos < sizeof(input_buffer) - 1)
    {
        if (input_buffer[input_pos] != '\0')
        {
            size_t len = input_pos;
            while (input_buffer[len])
                len++;
            for (size_t i = len + 1; i > input_pos; --i)
                input_buffer[i] = input_buffer[i - 1];
        }
        input_buffer[input_pos] = ascii;
        input_pos++;
        redraw_input();
    }
}