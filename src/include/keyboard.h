// src/include/keyboard.h
#ifndef NYRIX_KEYBOARD_H
#define NYRIX_KEYBOARD_H

#include <stdint.h>

class Keyboard
{
public:
    static void init();
    static void handle_interrupt();
    static void handle_scancode(uint8_t scancode); // <-- ДОБАВЬТЕ
};
#endif
