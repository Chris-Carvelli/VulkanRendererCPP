#include <cc_profiler.h>

#include <cc_logger.h>
#include <cc_allocator.h>

#include <time.h>

const uint32_t             MAX_SAMPLE_HANDLES_COUNT = 1024;
const HandleProfilerSample NULL_SAMPLE_HANDLE = (HandleProfilerSample)-1; // force this to biggest possible key

// `HandleProfilerSample` is an index in the data arrays
typedef struct Profiler {
	BumpAllocator *allocator;
	uint32_t num_samples;
	uint32_t samples_stack_depth;

	const char** names;
	uint32_t* counts;

	uint64_t* aggregate_times; // `long` here is implementation-defined in `time.h`, so we can't use a fixed-lenght integer
	struct timespec* last_timestamps; // `long` here is implementation-defined in `time.h`, so we can't use a fixed-lenght integer

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
	handle->allocator           = allocator;
	handle->num_samples         = 0;
	handle->samples_stack_depth = 0;
	handle->names               = (const char**)     allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(const char*));
	handle->counts              = (uint32_t*)        allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint32_t));
	handle->aggregate_times     = (uint64_t*)        allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(uint64_t));
	handle->last_timestamps     = (struct timespec*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(struct timespec));

	// most of these will be unused most of the time, but it's still useful to have the full `MAX_SAMPLE_HANDLES_COUNT`
	// as depth for profiling recursive functions
	handle->sample_handles_stack = (HandleProfilerSample*) allocator_alloc_n(allocator, MAX_SAMPLE_HANDLES_COUNT, sizeof(HandleProfilerSample));
}

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin(Profiler* handle, HandleProfilerSample handle_sample) {
	CC_ASSERT(handle->samples_stack_depth < MAX_SAMPLE_HANDLES_COUNT - 1, "sample stack max depth (%d) reached", MAX_SAMPLE_HANDLES_COUNT)

	if (handle_sample == NULL_SAMPLE_HANDLE) {
		CC_ASSERT(handle->num_samples < MAX_SAMPLE_HANDLES_COUNT - 1, "MAX sample handles count (%d) exceeded", MAX_SAMPLE_HANDLES_COUNT)
		handle_sample = handle->num_samples;
		handle->names[handle_sample] = NULL;
		handle->counts[handle_sample] = 0;
		handle->aggregate_times[handle_sample] = 0;

		++handle->num_samples;
	}
	CC_ASSERT((handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0), "invalid handle")

	timespec_get(&handle->last_timestamps[handle_sample], TIME_UTC);

	handle->sample_handles_stack[handle->samples_stack_depth] = handle_sample;
	handle->samples_stack_depth++;
	
	return handle_sample;
}

// start a new sample
// call with `NULL_SAMPLE_HANDLE` to creeate a new sample
// returns the handle of the new sample
HandleProfilerSample profiler_sample_begin_named(Profiler* handle, HandleProfilerSample handle_sample, const char* name) {
	CC_ASSERT(handle->samples_stack_depth < MAX_SAMPLE_HANDLES_COUNT - 1, "sample stack max depth (%d) reached", MAX_SAMPLE_HANDLES_COUNT)

	if (handle_sample == NULL_SAMPLE_HANDLE) {
		CC_ASSERT(handle->num_samples < MAX_SAMPLE_HANDLES_COUNT - 1, "MAX sample handles count (%d) exceeded", MAX_SAMPLE_HANDLES_COUNT)
		handle_sample = handle->num_samples;
		handle->names[handle_sample] = (const char*)allocator_put_str(handle->allocator, name);
		handle->counts[handle_sample] = 0;
		handle->aggregate_times[handle_sample] = 0;

		++handle->num_samples;
	}
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle")

	timespec_get(&handle->last_timestamps[handle_sample], TIME_UTC);

	handle->sample_handles_stack[handle->samples_stack_depth] = handle_sample;
	handle->samples_stack_depth++;

	return handle_sample;
}

void profiler_sample_end(Profiler* handle) {
	CC_ASSERT(handle->samples_stack_depth > 0, "trying to end sample when none have been started")
	HandleProfilerSample handle_sample = handle->sample_handles_stack[handle->samples_stack_depth - 1];
	handle->samples_stack_depth--;

	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle")

	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	
	ts.tv_sec -= handle->last_timestamps[handle_sample].tv_sec;
	ts.tv_nsec -= handle->last_timestamps[handle_sample].tv_nsec;
	if (ts.tv_nsec < 0) {
		ts.tv_nsec += 1000000000;
		ts.tv_sec--;
	}

	// duration can't be negative, so this cast should be safe
	handle->aggregate_times[handle_sample] += (uint64_t)(ts.tv_nsec + ts.tv_sec * 1000000000);
	handle->last_timestamps[handle_sample] = ts;
	handle->counts[handle_sample]++;
}

void profiler_data_print(Profiler* handle) {
	const uint32_t BUFFER_SIZE = 64;
	char buf_avg[BUFFER_SIZE];
	char buf_tot[BUFFER_SIZE];

	CC_LOG(CC_IMPORTANT, "Name\t\ttot. time\tcount\tavg. time");
	for(uint32_t i = 0; i < handle->num_samples; ++i)
	{

		format_time(handle->aggregate_times[i], buf_tot, BUFFER_SIZE);
		format_time(handle->aggregate_times[i] / handle->counts[i], buf_avg, BUFFER_SIZE);

		CC_LOG(
			CC_INFO,
			"%s\t%s\t%d\t%s",
			handle->names[i],
			buf_tot,
			handle->counts[i],
			buf_avg
		);
	}
}

ProfilerSample profiler_data_get(Profiler* handle, HandleProfilerSample handle_sample) {
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle")
	
	return (ProfilerSample) {
		.aggregate_time = handle->aggregate_times[handle_sample],
		.count          = handle->counts[handle_sample]
	};
}

const char *profiler_data_get_name(Profiler* handle, HandleProfilerSample handle_sample) {
	CC_ASSERT(handle_sample < MAX_SAMPLE_HANDLES_COUNT && handle_sample >= 0, "invalid handle")

	return handle->names[handle_sample];
}
