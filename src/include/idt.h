// src/include/idt.h
#ifndef NYRIX_IDT_H
#define NYRIX_IDT_H

#include <stdint.h>

struct IDTEntry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct IDTPtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

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

// Глобальная таблица IDT (нужна для paging_init_full)
extern IDTEntry idt_entries[];

#endif