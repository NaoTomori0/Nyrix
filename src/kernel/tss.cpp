#include "tss.h"
#include <stddef.h>

static TSSEntry tss;

void tss_init(uint32_t kernel_stack_top) {
    uint8_t* p = (uint8_t*)&tss;
    for (size_t i = 0; i < sizeof(TSSEntry); ++i) p[i] = 0;
    tss.ss0  = 0x10;
    tss.esp0 = kernel_stack_top;
}