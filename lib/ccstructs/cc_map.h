#ifndef CC_MAP_H
#define CC_MAP_H

#include <stdint.h>

// foward declarations
typedef struct BumpAllocator BumpAllocator;

// declarations
extern const uint8_t KEY_NOT_FOUND;
extern const uint8_t KEY_FOUND;

typedef int (*fn_compare_t)(const void* a, const void* b);
typedef struct Map Map;

Map*    map_make(size_t num_elements, size_t size_key, size_t size_value, fn_compare_t fn_comparator, size_t max_size);
uint8_t map_put(Map* handle, void* key, void* value);
uint8_t map_get(Map* handle, void* key, void* value);
uint8_t map_remove(Map* handle, void* key, void* value);
void    map_destroy(Map* handle);

uint32_t map_diagnostics_count_kvps(Map* handle);
void     map_diagnostics_print_buckets(Map* handle);

#endif