#ifndef CC_MAP_GENERIC_H
#define CC_MAP_GENERIC_H

#include <stdint.h>

// foward declarations
typedef struct BumpAllocator BumpAllocator;

// declarations
typedef int (*fn_compare_t)(const void* a, const void* b);

extern const uint8_t KEY_NOT_FOUND;
extern const uint8_t KEY_FOUND;

typedef struct MapFixed MapFixed;

MapFixed* map_fixed_make(size_t num_elements, fn_compare_t fn_comparator, size_t max_size);

uint8_t map_fixed_put(MapFixed* handle, uint32_t key, uint32_t value);
uint8_t map_fixed_get(MapFixed* handle, uint32_t key, uint32_t* value);
void    map_fixed_destroy(MapFixed* handle);

void map_fixed_diagnostics_print_buckets(MapFixed* handle);

// **************************************************************
// c file
// **************************************************************


// ****************************************************************************
// fixed types map
// ****************************************************************************



#endif