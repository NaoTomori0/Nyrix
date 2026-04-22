// src/include/gdt.h
#ifndef NYRIX_GDT_H
#define NYRIX_GDT_H

#include <stdint.h>

// Структура дескриптора сегмента (8 байт)
struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

// Структура для инструкции lgdt
struct GDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

class GDT {
   public:
    static void init();
};

#endif
