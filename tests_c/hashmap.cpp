#include <map>

#include <memory.h>
#include <stdint.h>

// #define EXCLUDE_LOOP

const int NUM_ELEMENTS =4;
const int HASHMAP_SIZE = 4;
const int N_TRIALS = 10;

extern "C" {
	#include <cc_logger.h>
	#include <cc_map.h>
	#include <cc_allocator.h>
	#include <cc_profiler.h>
}

int cmp_integers(const void* a, const void* b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
}

void validate_get(uint32_t* values, const int NUM_ELEMENTS, const char* trial) {
	int num_errors = 0;
	for(int i = 0; i < NUM_ELEMENTS; ++i)
		if(values[i] != i) {
			CC_LOG(CC_WARNING, "%3d: expected: %3d    retrieved: %3d", i, i, values[i]);
			++num_errors;
		}
	if (num_errors > 0)
	CC_LOG(CC_ERROR, "%s failed to retrieve correct values (%d errors)", trial, num_errors);
}

void do_trials(uint32_t* keys, uint32_t* values, uint32_t* values_retrieved);

int main() {

	srand(4243);

	uint32_t* keys = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values_retrieved = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i) {
		keys[i] = i;
		values[i] = i;
	}

	for(int i = 0; i < N_TRIALS; ++i)
		do_trials(keys, values, values_retrieved);

	return 0;
}

void do_trials(uint32_t* keys, uint32_t* values, uint32_t* values_retrieved) {
	// shuffle keys
	for(uint32_t i = NUM_ELEMENTS - 1; i > 0; --i) {
		uint32_t j = rand() % i;

		uint32_t swap = keys[i];
		keys[i] = keys[j];
		keys[j] = swap;
	}

	BumpAllocator* allocator = allocator_make_bump(MB(4));
	Profiler* profiler = profiler_shared_create(allocator);
	HandleProfilerSample h_put_base = profiler_create_sample_handle_named(profiler, "[put]base");
	HandleProfilerSample h_get_base = profiler_create_sample_handle_named(profiler, "[get]base");
	HandleProfilerSample h_put_std = profiler_create_sample_handle_named(profiler, "[put]std");
	HandleProfilerSample h_get_std = profiler_create_sample_handle_named(profiler, "[get]std");

	Map* a = map_make(HASHMAP_SIZE, sizeof(uint32_t), MB(512));
	std::map<uint32_t, uint32_t> b;

#ifdef EXCLUDE_LOOP
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE(profiler, h_put_base, map_put(a, &keys[i], &i); )
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE(profiler, h_get_base, map_get(a, &keys[i], &values_retrieved[i]); )
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE(profiler, h_put_std, b[keys[i]] = i; )
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE(profiler, h_get_std, values_retrieved[i] = b[keys[i]]; )
#else
	PROFILE(profiler, h_put_base, 
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_put(a, &keys[i], sizeof(uint32_t), &i);
	)

	memset(values_retrieved, 0, NUM_ELEMENTS * sizeof(uint32_t));
	PROFILE(profiler, h_get_base, 
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_get(a, &keys[i], sizeof(uint32_t), &values_retrieved[i]);
	)
	validate_get(values_retrieved, NUM_ELEMENTS, "cc base");

	PROFILE(profiler, h_put_std, 
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			b[keys[i]] = i;
	)

	memset(values_retrieved, 0, NUM_ELEMENTS * sizeof(uint32_t));
	PROFILE(profiler, h_get_std, 
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			values_retrieved[i] = b[keys[i]];
	)
	validate_get(values_retrieved, NUM_ELEMENTS, "std");
#endif

	profiler_data_print(profiler);

	profiler_shared_destroy(profiler);
	allocator_free_bump(allocator);
}