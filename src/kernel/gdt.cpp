#include "gdt.h"
#include "tss.h"

static GDTEntry gdt_entries[6]; // нулевая, код ядра, данные ядра, код пользователя, данные пользователя, TSS
static GDTPtr gdt_ptr;

extern "C" void gdt_flush(uint32_t);

void GDT::init()
{
    // 1. Нулевой дескриптор
    gdt_entries[0] = {0, 0, 0, 0, 0, 0};
    // 2. Код ядра (Ring 0)
    gdt_entries[1] = {0xFFFF, 0, 0, 0x9A, 0xCF, 0};
    // 3. Данные ядра (Ring 0)
    gdt_entries[2] = {0xFFFF, 0, 0, 0x92, 0xCF, 0};
    // 4. Код пользователя (Ring 3) – селектор 0x1B
    gdt_entries[3] = {0xFFFF, 0, 0, 0xFA, 0xCF, 0};
    // 5. Данные пользователя (Ring 3) – селектор 0x23
    gdt_entries[4] = {0xFFFF, 0, 0, 0xF2, 0xCF, 0};

    // 6. TSS – селектор 0x28
    uint32_t tss_base = (uint32_t)&tss_entry;
    gdt_entries[5].limit_low = sizeof(TSSEntry) - 1;
    gdt_entries[5].base_low = tss_base & 0xFFFF;
    gdt_entries[5].base_middle = (tss_base >> 16) & 0xFF;
    gdt_entries[5].access = 0x89;      // Present, 32-bit TSS (available)
    gdt_entries[5].granularity = 0x00; // гранулярность = 1 байт
    gdt_entries[5].base_high = (tss_base >> 24) & 0xFF;

    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = reinterpret_cast<uint32_t>(&gdt_entries);
    gdt_flush(reinterpret_cast<uint32_t>(&gdt_ptr));

    // Загружаем Task Register (селектор 0x28)
    tss_flush(0x28);
}