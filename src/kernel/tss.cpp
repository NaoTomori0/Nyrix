// src/kernel/tss.cpp
#include "tss.h"
#include <stddef.h>

TSSEntry tss_entry; // глобальная переменная (без static)

void tss_init(uint32_t kernel_stack_top)
{
    uint8_t *p = (uint8_t *)&tss_entry;
    for (size_t i = 0; i < sizeof(TSSEntry); ++i)
        p[i] = 0;
    tss_entry.ss0 = 0x10; // сегмент данных ядра
    tss_entry.esp0 = kernel_stack_top;
}

void tss_flush(uint16_t selector)
{
    __asm__ volatile("ltr %0" : : "r"(selector));
}