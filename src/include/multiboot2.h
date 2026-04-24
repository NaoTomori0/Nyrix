#ifndef NYRIX_MULTIBOOT2_H
#define NYRIX_MULTIBOOT2_H

#include <stdint.h>

struct multiboot2_tag
{
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

#define MULTIBOOT2_TAG_TYPE_MMAP 6
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 8 // <-- добавлено

struct multiboot2_tag_mmap
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

struct multiboot2_mmap_entry
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot2_tag_framebuffer
{ // <-- добавлено
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

#endif