#ifndef CC_ALLOCATOR_H
#define CC_ALLOCATOR_H

#include <stddef.h>

#define BIT(x) 1 << (x)
#define KB(x) ((unsigned long long)1024 * x)
#define MB(x) ((unsigned long long)1024 * KB(x))
#define GB(x) ((unsigned long long)1024 * MB(x))

typedef struct BumpAllocator BumpAllocator;

BumpAllocator* allocator_make_bump(size_t size);
void           allocator_free_bump(BumpAllocator *bump_allocator);

// allocates `size` space on the allocator
void* allocator_alloc(BumpAllocator* bump_allocator, size_t size);

// allocates `count * size` space on the allocator
void* allocator_alloc_n(BumpAllocator* bump_allocator, size_t count, size_t size);

// allocates space for (string + terminator) and copies string
void* allocator_put_str(BumpAllocator* bump_allocator, const char * str);

void* allocator_peek (BumpAllocator* bump_allocator);
void  allocator_reset(BumpAllocator* bump_allocator);

// makes allocator behave like a stack allocator
void  allocator_pop(BumpAllocator* bump_allocator, size_t size);

// fill allocator memory with recognizeable bit pattern
void  allocator_debug_pattern_fill(BumpAllocator* bump_allocator, int pattern);
void* allocator_debug_wrapper(BumpAllocator* bump_allocator, size_t size, const char* file, int line);


#endif