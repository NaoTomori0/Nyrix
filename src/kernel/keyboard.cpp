// src/kernel/keyboard.cpp
#include "keyboard.h"

#include "idt.h"  // для inb/outb

// Внешняя функция вывода символа (определена в kernel.cpp)
extern void terminal_putchar(char c);

// Простая раскладка US QWERTY для скан-кодов (без модификаторов)
static const char scancode_to_ascii[128] = {
    0,    0,    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']',
    '\n', 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,    '*',
    0,    ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,    0,
    0,    '+',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,
    0,    0,    0,   0,   0,   0,   0};

// Состояние клавиш-модификаторов
static bool shift_pressed = false;
static bool caps_lock = false;

void Keyboard::init() {
    // Пока ничего дополнительно настраивать не нужно.
    // Контроллер клавиатуры уже работает в режиме совместимости.
}

void Keyboard::handle_interrupt() {
    // Читаем скан-код из порта 0x60
    uint8_t scancode = inb(0x60);

    // Обработка нажатия или отпускания
    if (scancode & 0x80) {
        // Клавиша отпущена (старший бит = 1)
        scancode &= 0x7F;
        if (scancode == 0x2A ||
            scancode == 0x36) {  // Left Shift или Right Shift
            shift_pressed = false;
        }
        // Другие модификаторы можно добавить позже
        return;
    }

    // Клавиша нажата
    // Проверяем модификаторы
    if (scancode == 0x2A || scancode == 0x36) {  // Shift
        shift_pressed = true;
        return;
    }
    if (scancode == 0x3A) {  // Caps Lock (toggle)
        caps_lock = !caps_lock;
        return;
    }

    // Преобразуем скан-код в ASCII
    char ascii = scancode_to_ascii[scancode];
    if (ascii == 0) {
        return;  // Неизвестная клавиша
    }

    // Учитываем Shift и Caps Lock для букв
    if (ascii >= 'a' && ascii <= 'z') {
        bool should_capitalize = (shift_pressed ^ caps_lock);
        if (should_capitalize) {
            ascii -= 32;  // В верхний регистр
        }
    } else if (shift_pressed) {
        // Обработка символов, которые меняются с Shift (цифры, знаки)
        switch (ascii) {
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

    // Выводим символ на экран
    terminal_putchar(ascii);
}
