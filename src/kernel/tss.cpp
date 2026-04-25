#include "tss.h"
#include <stddef.h>

TSSEntry tss_entry;

void tss_init(uint32_t kernel_stack_top)
{
    // Обнуляем структуру
    for (size_t i = 0; i < sizeof(TSSEntry); ++i)
        ((uint8_t *)&tss_entry)[i] = 0;
    tss_entry.ss0 = 0x10;              // сегмент данных ядра
    tss_entry.esp0 = kernel_stack_top; // стек, используемый при входе из Ring 3
}

void tss_flush(uint16_t selector)
{
    __asm__ volatile("ltr %0" : : "r"(selector));
}