#include <cc_allocator.h>

#include <cc_logger.h>


#include <string.h> // for strlen and strcpy_s
#include <memory.h> // for memset

typedef struct BumpAllocator {
    size_t capacity;
    size_t used;
    void *memory;
} BumpAllocator;

BumpAllocator *allocator_make_bump(size_t size) {
    BumpAllocator *ba = (BumpAllocator*)malloc(sizeof(*ba));

    ba->capacity = size;
    ba->used = 0;
    ba->memory = malloc(size);

    CC_ASSERT(ba->memory, "Failed to allocate memory");

    memset(ba->memory, 0, size);
    return ba;
}

void allocator_free_bump(BumpAllocator *bump_allocator) {
    free(bump_allocator->memory);
    free(bump_allocator);
}

void* allocator_alloc(BumpAllocator *bump_allocator, size_t size) {
    void *ret = NULL;
	size_t aligned_size = (size + 7) & ~ (size_t)7; // This makes sure the first 4 bits are set to 0
	// ^^^ CHECK THIS ONE (compatibility, especially x86/x86_64) ^^^

    CC_ASSERT(bump_allocator->used + aligned_size <= bump_allocator->capacity, "Bump allocator is full");
	
	ret = (char*)bump_allocator->memory + bump_allocator->used;
	bump_allocator->used += aligned_size;

	return ret;
}

void* allocator_alloc_n(BumpAllocator* bump_allocator, size_t count, size_t size)
{
    return allocator_alloc(bump_allocator, count * size);
}

void* allocator_put_str(BumpAllocator* bump_allocator, const char* str) {
    size_t size = strlen(str) + 1; // +1 for string terminator
    void* pointer = allocator_alloc(bump_allocator, size);

    strcpy_s((char *)pointer, size, str);
    return pointer;
}

void* allocator_peek(BumpAllocator* bump_allocator) {
    return (char*)bump_allocator->memory + bump_allocator->used;
}

void allocator_reset(BumpAllocator* bump_allocator) {
    bump_allocator->used = 0;
}

void allocator_pop(BumpAllocator* bump_allocator, size_t size) {
    bump_allocator->used -= size;
}

void allocator_debug_status(BumpAllocator* bump_allocator) {
    const size_t size = 32;
    char buf_capacity[size];
    char buf_used[size];
    char buf_left[size];
    double perc_used = (double)bump_allocator->used / (double)bump_allocator->capacity;

    format_size(bump_allocator->capacity, buf_capacity, size);
    format_size(bump_allocator->used, buf_used, size);
    format_size(bump_allocator->capacity - bump_allocator->used, buf_left, size);
    CC_LOG(CC_INFO, "capacity: %s    used: %s (%3.2f)    left: %s", buf_capacity, buf_used, perc_used, buf_left);
}

void allocator_debug_pattern_fill(BumpAllocator* bump_allocator, int pattern) {
    memset(bump_allocator->memory, pattern, bump_allocator->capacity - bump_allocator->used);
}
