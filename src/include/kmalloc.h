// src/include/kmalloc.h
#ifndef NYRIX_KMALLOC_H
#define NYRIX_KMALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void  kmalloc_init(void);
void* kmalloc(size_t size);
void  kfree(void* ptr);
void* krealloc(void* ptr, size_t size);   // опционально, пока можно без неё

#ifdef __cplusplus
}
#endif

#endif