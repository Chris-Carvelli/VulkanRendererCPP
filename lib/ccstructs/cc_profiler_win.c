#ifdef _WIN32 // TODO keep one log and change timing functions only

#include <Windows.h> // for QueryPerfCounter

#include <cc_profiler.h>

#include <cc_logger.h>
#include <cc_allocator.h>


const uint32_t             MAX_SAMPLE_HANDLES_COUNT = 1024;
const HandleProfilerSample NULL_SAMPLE_HANDLE = (HandleProfilerSample)-1; // force this to biggest possible key

// `HandleProfilerSample` is an index in the data arrays
typedef struct Profiler {
	BumpAllocator *allocator;
	uint32_t num_samples;
	uint32_t samples_stack_depth;
	LARGE_INTEGER system_frequency;

	const char** names;
	uint32_t* counts;

	LARGE_INTEGER* aggregate_times;
	LARGE_INTEGER* last_timestamps;

	HandleProfilerSample* sample_handles_stack;
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
	handle->names               = (const char**)   allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(const char*));
	handle->counts              = (uint32_t*)      allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint32_t));
	handle->aggregate_times     = (LARGE_INTEGER*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(LARGE_INTEGER));
	handle->last_timestamps     = (LARGE_INTEGER*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(LARGE_INTEGER));

	// most of these will be unused most of the time, but it's still useful to have the full `MAX_SAMPLE_HANDLES_COUNT`
	// as depth for profiling recursive functions
	handle->sample_handles_stack = (HandleProfilerSample*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(HandleProfilerSample));

	// zero necessary memory
	memset(handle->aggregate_times, 0, MAX_SAMPLE_HANDLES_COUNT * sizeof(LARGE_INTEGER));
}

HandleProfilerSample profiler_create_sample_handle(Profiler* handle) {
	CC_ASSERT(handle->samples_stack_depth < MAX_SAMPLE_HANDLES_COUNT - 1, "sample stack max depth (%d) reached", MAX_SAMPLE_HANDLES_COUNT);
	CC_ASSERT(handle->num_samples < MAX_SAMPLE_HANDLES_COUNT - 1, "MAX sample handles count (%d) exceeded", MAX_SAMPLE_HANDLES_COUNT);

	HandleProfilerSample handle_sample = handle->num_samples;
	handle->names[handle_sample] = NULL;
	handle->counts[handle_sample] = 0;

	++handle->num_samples;

	return handle_sample;
}

HandleProfilerSample profiler_create_sample_handle_named(Profiler* handle, const char* name) {
	HandleProfilerSample handle_sample = profiler_create_sample_handle(handle);
	
	handle->names[handle_sample] = (const char*)allocator_put_str(handle->allocator, name);

	return handle_sample;
}

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin(Profiler* handle, HandleProfilerSample handle_sample) {
	if (handle_sample == NULL_SAMPLE_HANDLE)
		handle_sample = profiler_create_sample_handle(handle);

	CC_ASSERT((handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0), "invalid handle");
	
	handle->sample_handles_stack[handle->samples_stack_depth] = handle_sample;
	handle->samples_stack_depth++;
	QueryPerformanceCounter(&handle->last_timestamps[handle_sample]);

	return handle_sample;
}

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin_named(Profiler* handle, HandleProfilerSample handle_sample, const char* name) {
	if (handle_sample == NULL_SAMPLE_HANDLE)
		handle_sample = profiler_create_sample_handle_named(handle, name);
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle");


	handle->sample_handles_stack[handle->samples_stack_depth] = handle_sample;
	handle->samples_stack_depth++;
	QueryPerformanceCounter(&handle->last_timestamps[handle_sample]);

	return handle_sample;
}

void profiler_sample_end(Profiler* handle) {
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	CC_ASSERT(handle->samples_stack_depth > 0, "trying to end sample when none have been started");
	HandleProfilerSample handle_sample = handle->sample_handles_stack[handle->samples_stack_depth - 1];
	handle->samples_stack_depth--;

	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle");


	// duration can't be negative, so this cast should be safe
	handle->aggregate_times[handle_sample].QuadPart += end.QuadPart - handle->last_timestamps[handle_sample].QuadPart;
	handle->last_timestamps[handle_sample] = end;
	handle->counts[handle_sample]++;
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

ProfilerSample profiler_data_get(Profiler* handle, HandleProfilerSample handle_sample) {
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle");

		return (ProfilerSample) {
			.aggregate_time = (uint64_t)handle->aggregate_times[handle_sample].QuadPart,
			.count          = handle->counts[handle_sample]
	};
}

const char *profiler_data_get_name(Profiler* handle, HandleProfilerSample handle_sample) {
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle");

		return handle->names[handle_sample];
}
#endif