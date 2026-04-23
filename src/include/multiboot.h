// src/include/multiboot.h
#ifndef NYRIX_MULTIBOOT_H
#define NYRIX_MULTIBOOT_H

#include <stdint.h>

// Основная структура, передаваемая GRUB
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    // ... (символы ELF)
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

// Запись карты памяти
struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;  // 1 = доступная память
} __attribute__((packed));

#endif
