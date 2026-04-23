// src/kernel/gdt.cpp
#include "gdt.h"
#include "tss.h"

static GDTEntry gdt_entries[6];
static GDTPtr gdt_ptr;
static TSSEntry tss_entry;

extern "C" void gdt_flush(uint32_t);

void GDT::init()
{
    // 1. Нулевой дескриптор
    gdt_entries[0].limit_low = 0;
    gdt_entries[0].base_low = 0;
    gdt_entries[0].base_middle = 0;
    gdt_entries[0].access = 0;
    gdt_entries[0].granularity = 0;
    gdt_entries[0].base_high = 0;

    // 2. Код ядра (Ring 0)
    gdt_entries[1].limit_low = 0xFFFF;
    gdt_entries[1].base_low = 0;
    gdt_entries[1].base_middle = 0;
    gdt_entries[1].access = 0x9A;
    gdt_entries[1].granularity = 0xCF;
    gdt_entries[1].base_high = 0;

    // 3. Данные ядра (Ring 0)
    gdt_entries[2].limit_low = 0xFFFF;
    gdt_entries[2].base_low = 0;
    gdt_entries[2].base_middle = 0;
    gdt_entries[2].access = 0x92;
    gdt_entries[2].granularity = 0xCF;
    gdt_entries[2].base_high = 0;

    // 4. Код пользователя (Ring 3) – селектор 0x1B
    gdt_entries[3].limit_low = 0xFFFF;
    gdt_entries[3].base_low = 0;
    gdt_entries[3].base_middle = 0;
    gdt_entries[3].access = 0xFA;
    gdt_entries[3].granularity = 0xCF;
    gdt_entries[3].base_high = 0;

    // 5. Данные пользователя (Ring 3) – селектор 0x23
    gdt_entries[4].limit_low = 0xFFFF;
    gdt_entries[4].base_low = 0;
    gdt_entries[4].base_middle = 0;
    gdt_entries[4].access = 0xF2;
    gdt_entries[4].granularity = 0xCF;
    gdt_entries[4].base_high = 0;

    // 6. TSS – селектор 0x28
    uint32_t tss_base = (uint32_t)&tss_entry;
    gdt_entries[5].limit_low = sizeof(TSSEntry) - 1;
    gdt_entries[5].base_low = tss_base & 0xFFFF;
    gdt_entries[5].base_middle = (tss_base >> 16) & 0xFF;
    gdt_entries[5].access = 0x89;      // Present, 32-bit TSS (available)
    gdt_entries[5].granularity = 0x00; // лимит 0, старшие биты базы
    gdt_entries[5].base_high = (tss_base >> 24) & 0xFF;

    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = reinterpret_cast<uint32_t>(&gdt_entries);
    gdt_flush(reinterpret_cast<uint32_t>(&gdt_ptr));

    // Загружаем TSS (селектор 0x28)
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}