#ifndef CC_PROFILER_H
#define CC_PROFILER_H

#include <stdint.h>

typedef struct BumpAllocator BumpAllocator;

typedef struct Profiler Profiler;

extern const uint32_t MAX_SAMPLE_HANDLES_COUNT;

typedef struct {
	uint64_t aggregate_time;
	uint32_t count;
} ProfilerSample;

void     profiler_init(void);

void     profiler_sample_begin(const char* id);
void     profiler_sample_end(void);

void     profiler_highperf_begin(void);
uint64_t profiler_highperf_end(void);
uint64_t profiler_highperf_sample(void);

void     profiler_data_print(void);

#define PROFILE(id, scope)     \
{                              \
    profiler_sample_begin(id); \
    scope                      \
    profiler_sample_end();     \
}                              \

#endif