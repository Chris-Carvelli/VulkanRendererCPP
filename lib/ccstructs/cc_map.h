#ifndef CC_MAP_H
#define CC_MAP_H

#include <stddef.h>
#include <stdint.h>

// foward declarations
typedef struct BumpAllocator BumpAllocator;

// declarations
extern const uint64_t KEY_NOT_FOUND;
extern const uint64_t KEY_FOUND;

typedef int (*fn_compare_t)(const void* a, const void* b);
typedef struct Map Map;

Map*     map_make(BumpAllocator* allocator, size_t num_elements, size_t size_value);
uint64_t map_put(Map* handle, const void* key, size_t key_size, const void* value);
uint64_t map_get(Map* handle, const void* key, size_t key_size, void* value);
uint64_t map_del(Map* handle, const void* key, size_t key_size, void* value);
void     map_destroy(Map* handle);

uint64_t map_diagnostics_count_kvps(Map* handle);
void     map_diagnostics_print_buckets(Map* handle);

#endif