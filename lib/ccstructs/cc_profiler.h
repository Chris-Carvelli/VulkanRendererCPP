#ifndef CC_PROFILER_H
#define CC_PROFILER_H

#include <stdint.h>

typedef struct BumpAllocator BumpAllocator;

typedef struct Profiler Profiler;

extern const uint32_t             MAX_SAMPLE_HANDLES_COUNT;

typedef struct {
	uint64_t aggregate_time;
	uint32_t count;
} ProfilerSample;

Profiler* profiler_shared_create(BumpAllocator *allocator);
void      profiler_shared_destroy(Profiler* handle);
void      profiler_owned_create(BumpAllocator *allocator, Profiler *handle);

// start a new sample
void profiler_sample_begin(Profiler* handle, char* id);
void profiler_sample_end(Profiler* handle);

void profiler_data_print(Profiler* handle);

#define PROFILE(profiler, id, scope)     \
{                                        \
    profiler_sample_begin(profiler, id); \
    scope                                \
    profiler_sample_end(profiler);       \
}

#endif