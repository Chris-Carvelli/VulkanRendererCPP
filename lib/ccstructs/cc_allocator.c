#include <cc_allocator.h>

#include <cc_logger.h>



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
    if(!ba->memory) {
    CC_ASSERT(1, "Failed to allocate memory")
        CC_LOG_SYS_ERROR();
        exit(3);
    }
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

    CC_ASSERT(bump_allocator->used + aligned_size <= bump_allocator->capacity, "Bump allocator is full")
	
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

void* allocator_debug_wrapper(BumpAllocator* bump_allocator, size_t size, const char* file, int line) {
    printf("[%d] %s\n", line, file);
    return allocator_alloc(bump_allocator, size);
}