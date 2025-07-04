#ifdef _WIN32 // TODO keep one log and change timing functions only

#include <Windows.h> // for QueryPerfCounter

#include <cc_profiler.h>

#include <cc_map.h>
#include <cc_logger.h>
#include <cc_allocator.h>


const uint32_t  MAX_SAMPLE_HANDLES_COUNT = 1024;

// local functions
static uint32_t create_sample_handle(Profiler* handle, char* id, size_t size);

// `HandleProfilerSample` is an index in the data arrays
typedef struct Profiler {
	BumpAllocator *allocator;
	uint32_t num_samples;
	uint32_t samples_stack_depth;
	LARGE_INTEGER system_frequency;

	Map* map;

	const char** names;
	uint32_t*    counts;
	LARGE_INTEGER* aggregate_times;
	LARGE_INTEGER* last_timestamps;

	uint32_t* sample_handles_stack;
} Profiler;

Profiler* profiler_shared_create(BumpAllocator *allocator) {
	Profiler *handle = (Profiler*)allocator_alloc(allocator, sizeof(Profiler));

	profiler_owned_create(allocator, handle);
	return handle;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

void profiler_shared_destroy(Profiler* handle) {

}

#pragma clang diagnostic pop

void profiler_owned_create(BumpAllocator *allocator, Profiler *handle) {
	// TODO warn about minimum memory requirements for profiler
	// also, evaluate if profiler should be allowed to manage its own memory (seems reasonable)
	/*char buf[32];
	format_size(MAX_SAMPLE_HANDLES_COUNT * (sizeof(const char*) + sizeof(uint32_t) + sizeof(LARGE_INTEGER) * 2), buf, 32);
	CC_LOG(CC_INFO, "necessary space: %s", buf);*/

	handle->allocator           = allocator;
	handle->num_samples         = 0;
	handle->samples_stack_depth = 0;

	QueryPerformanceFrequency(&handle->system_frequency);
	handle->map = map_make(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint32_t));

	handle->names               = (const char**)   allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(const char*));
	handle->counts              = (uint32_t*)      allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint32_t));
	handle->aggregate_times     = (LARGE_INTEGER*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(LARGE_INTEGER));
	handle->last_timestamps     = (LARGE_INTEGER*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(LARGE_INTEGER));

	// most of these will be unused most of the time, but it's still useful to have the full `MAX_SAMPLE_HANDLES_COUNT`
	// as depth for profiling recursive functions
	handle->sample_handles_stack = (uint32_t*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint32_t));

	// zero necessary memory
	memset(handle->aggregate_times, 0, MAX_SAMPLE_HANDLES_COUNT * sizeof(LARGE_INTEGER));
}

// start a new sample
void profiler_sample_begin(Profiler* handle, char* id) {
	uint32_t idx;
	size_t len = strlen(id);
	if (map_get(handle->map, id, len, &idx) == KEY_NOT_FOUND)
		idx = create_sample_handle(handle, id, len);

	CC_ASSERT((idx < MAX_SAMPLE_HANDLES_COUNT && idx >= 0), "invalid handle");
	
	handle->sample_handles_stack[handle->samples_stack_depth] = idx;
	handle->samples_stack_depth++;
	QueryPerformanceCounter(&handle->last_timestamps[idx]);
}


void profiler_sample_end(Profiler* handle) {
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	CC_ASSERT(handle->samples_stack_depth > 0, "trying to end sample when none have been started");
	uint32_t idx = handle->sample_handles_stack[handle->samples_stack_depth - 1];
	handle->samples_stack_depth--;

	CC_ASSERT(idx < MAX_SAMPLE_HANDLES_COUNT && idx >= 0, "invalid handle");


	// duration can't be negative, so this cast should be safe
	handle->aggregate_times[idx].QuadPart += end.QuadPart - handle->last_timestamps[idx].QuadPart;
	handle->last_timestamps[idx] = end;
	handle->counts[idx]++;
}

void profiler_data_print(Profiler* handle) {
	CC_LOG(CC_IMPORTANT, "Name\t\ttot. time\tcount\tavg. time");
	for(uint32_t i = 0; i < handle->num_samples; ++i)
	{

		double time_tot = (double)handle->aggregate_times[i].QuadPart / (double)handle->system_frequency.QuadPart;
		double time_avg = ((double)handle->aggregate_times[i].QuadPart / (double)handle->system_frequency.QuadPart) / handle->counts[i];

		CC_LOG(
			CC_INFO,
			"%s\t%.6f\t%d\t%.6f",
			handle->names[i],
			time_tot,
			handle->counts[i],
			time_avg
		);
	}
}

static uint32_t create_sample_handle(Profiler* handle, char* id, size_t size) {
	CC_ASSERT(handle->samples_stack_depth < MAX_SAMPLE_HANDLES_COUNT - 1, "sample stack max depth (%d) reached", MAX_SAMPLE_HANDLES_COUNT);
	CC_ASSERT(handle->num_samples < MAX_SAMPLE_HANDLES_COUNT - 1, "MAX sample handles count (%d) exceeded", MAX_SAMPLE_HANDLES_COUNT);

	uint32_t idx_new = handle->num_samples;
	handle->names[idx_new] = (char*)allocator_put_str(handle->allocator, id);
	handle->counts[idx_new] = 0;
	map_put(handle->map, id, size, &idx_new);

	++handle->num_samples;

	return idx_new;
}
#endif