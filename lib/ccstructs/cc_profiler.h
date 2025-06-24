#ifndef CC_PROFILER_H
#define CC_PROFILER_H

#include <stdint.h>

typedef struct BumpAllocator BumpAllocator;

typedef struct Profiler Profiler;
typedef uint32_t HandleProfilerSample;


extern const uint32_t             MAX_SAMPLE_HANDLES_COUNT;
extern const HandleProfilerSample NULL_SAMPLE_HANDLE;

typedef struct {
	uint64_t aggregate_time;
	uint32_t count;
} ProfilerSample;

Profiler* profiler_shared_create(BumpAllocator *allocator);
void      profiler_shared_destroy(Profiler* handle);
void      profiler_owned_create(BumpAllocator *allocator, Profiler *handle);

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin(Profiler* handle, HandleProfilerSample handle_sample);

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin_named(Profiler* handle, HandleProfilerSample handle_sample, const char* name);

void profiler_sample_end(Profiler* handle);

void          profiler_data_print(Profiler* handle);
ProfilerSample profiler_data_get(Profiler* handle, HandleProfilerSample handle_sample);
const char*   profiler_data_get_name(Profiler* handle, HandleProfilerSample handle_sample);

#define PROFILE(profiler, handle, name, scope)       \
profiler_sample_begin_named(profiler, handle, name); \
scope                                                \
profiler_sample_end(profiler);

#endif