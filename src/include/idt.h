// src/include/idt.h
#ifndef NYRIX_IDT_H
#define NYRIX_IDT_H

#include <stdint.h>

// Структура дескриптора прерывания (шлюз)
struct IDTEntry
{
    uint16_t base_low;
    uint16_t selector; // сегмент кода (0x08)
    uint8_t zero;      // всегда 0
    uint8_t flags;     // тип шлюза, DPL, Present
    uint16_t base_high;
} __attribute__((packed));

struct IDTPtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Состояние процессора, сохраняемое при прерывании
struct Registers
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

class IDT
{
public:
    static void init();
};

// Вспомогательные функции для портов ввода-вывода
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
