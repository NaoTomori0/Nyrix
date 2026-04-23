// src/kernel/pit.cpp
#include "pit.h"
#include "idt.h"   // для outb

void pit_init(uint32_t freq) {
    uint32_t divisor = 1193180 / freq;
    outb(0x43, 0x36);                     // команда: канал 0, режим 3, двоичный счётчик
    outb(0x40, divisor & 0xFF);           // младший байт делителя
    outb(0x40, (divisor >> 8) & 0xFF);    // старший байт
}