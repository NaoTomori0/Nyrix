// src/kernel/kmalloc.cpp
#include "kmalloc.h"

#include <stdint.h>

#include "pmm.h"

struct AllocHeader {
    void* cache_id;  // идентификатор кэша (можно Cache*)
};

// Кэш хранит идентификатор (например, индекс)
struct CacheInfo {
    size_t block_size;       // полезный размер (без заголовка)
    size_t real_block_size;  // с заголовком
    uintptr_t* free_list;  // список свободных блоков (указатели хранятся внутри
                           // блоков)
    size_t total_pages;
};

// Конфигурация кэшей
static const size_t BLOCK_SIZES[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
static const size_t NUM_CACHES = sizeof(BLOCK_SIZES) / sizeof(BLOCK_SIZES[0]);

static CacheInfo caches[NUM_CACHES];

static void cache_grow(CacheInfo* cache) {
    void* page = pmm_alloc_page();
    if (!page) return;  // критично, но пока без обработки

    cache->total_pages++;
    size_t block_sz = cache->real_block_size;
    char* ptr = reinterpret_cast<char*>(page);
    size_t num_blocks = 4096 / block_sz;

    for (size_t i = 0; i < num_blocks; ++i) {
        uintptr_t* block = reinterpret_cast<uintptr_t*>(ptr);
        *block = reinterpret_cast<uintptr_t>(cache->free_list);  // next
        cache->free_list = block;
        ptr += block_sz;
    }
}

void kmalloc_init() {
    for (size_t i = 0; i < NUM_CACHES; ++i) {
        caches[i].block_size = BLOCK_SIZES[i];
        caches[i].real_block_size = BLOCK_SIZES[i] + sizeof(AllocHeader);
        caches[i].free_list = nullptr;
        caches[i].total_pages = 0;
        cache_grow(&caches[i]);
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;

    // Запрос больше максимального полезного размера кэша — выделяем страницами
    if (size > BLOCK_SIZES[NUM_CACHES - 1]) {
        // Упрощённо: отдаём целую страницу, на которой перед данными заголовок
        // size_t pages = (size + sizeof(AllocHeader) + 4095) / 4096;
        // Пока можем выделить только одну страницу, т.к. pmm_alloc_page даёт
        // одну
        void* page = pmm_alloc_page();
        if (!page) return nullptr;
        AllocHeader* header = reinterpret_cast<AllocHeader*>(page);
        header->cache_id = nullptr;  // признак "страница"
        return reinterpret_cast<void*>(header + 1);
    }

    // Ищем подходящий кэш
    for (size_t i = 0; i < NUM_CACHES; ++i) {
        if (caches[i].block_size >= size) {
            CacheInfo* cache = &caches[i];
            if (!cache->free_list) cache_grow(cache);
            if (!cache->free_list) return nullptr;

            uintptr_t* block = cache->free_list;
            cache->free_list =
                reinterpret_cast<uintptr_t*>(*block);  // снимаем с головы

            AllocHeader* header = reinterpret_cast<AllocHeader*>(block);
            header->cache_id = cache;  // запоминаем, откуда выделен
            return reinterpret_cast<void*>(header + 1);
        }
    }
    return nullptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    AllocHeader* header = reinterpret_cast<AllocHeader*>(ptr) - 1;
    if (header->cache_id == nullptr) {
        // Это была страница, выделенная напрямую
        pmm_free_page(header);  // header указывает на начало страницы
        return;
    }
    CacheInfo* cache = static_cast<CacheInfo*>(header->cache_id);
    uintptr_t* block = reinterpret_cast<uintptr_t*>(header);
    *block = reinterpret_cast<uintptr_t>(cache->free_list);
    cache->free_list = block;
}
