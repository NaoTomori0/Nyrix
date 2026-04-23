#ifndef NYRIX_MULTIBOOT2_H
#define NYRIX_MULTIBOOT2_H

#include <stdint.h>

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

#define MULTIBOOT2_TAG_TYPE_MMAP        6

struct multiboot2_tag_mmap {
    uint32_t type;          // 6
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;          // 1 = available
    uint32_t reserved;
} __attribute__((packed));

#endif