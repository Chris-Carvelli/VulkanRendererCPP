#include <map>

#include <memory.h>
#include <stdint.h>

extern "C" {
	#include <cc_logger.h>
	#include <cc_map.h>
	#include <cc_allocator.h>
	#include <cc_profiler.h>

	#include <TMP_map_fixed.h>
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

int main() {
	const int NUM_ELEMENTS = 4096 * 128;
	const int HASHMAP_SIZE = 4096 * 128;

	srand(4243);

	uint32_t* keys = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values_retrieved = (uint32_t*)calloc(NUM_ELEMENTS,sizeof(uint32_t));
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i) {
		keys[i] = i;
		values[i] = i;
	}

	// shuffle keys
	for(uint32_t i = NUM_ELEMENTS - 1; i > 0; --i) {
		uint32_t j = rand() % i;

		uint32_t swap = keys[i];
		keys[i] = keys[j];
		keys[j] = swap;
	}

	BumpAllocator* allocator = allocator_make_bump(MB(4));
	Profiler* profiler = profiler_shared_create(allocator);

	// cc map
	memset(values_retrieved, 0, NUM_ELEMENTS * sizeof(uint32_t));
	Map* a = map_make(HASHMAP_SIZE, sizeof(uint32_t), sizeof(uint32_t), cmp_integers, MB(512));
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]base",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_put(a, &keys[i], &i);
	)
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]base",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_get(a, &keys[i], &values_retrieved[i]);
	)
	validate_get(values_retrieved, NUM_ELEMENTS, "cc base");


	// cc map fixed
	memset(values_retrieved, 0, NUM_ELEMENTS * sizeof(uint32_t));
	MapFixed* c = map_fixed_make(HASHMAP_SIZE, cmp_integers, MB(512));
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]fixed",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_fixed_put(c, keys[i], i);
	)
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]fixed",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_fixed_get(c, keys[i], &values_retrieved[i]);
	)
	validate_get(values_retrieved, NUM_ELEMENTS, "cc fixed");

	// std map
	memset(values_retrieved, 0, NUM_ELEMENTS * sizeof(uint32_t));
	std::map<uint32_t, uint32_t> b;
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]std",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			b[keys[i]] = i;
	)
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]std",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			values_retrieved[i] = b[keys[i]];
	)
	validate_get(values_retrieved, NUM_ELEMENTS, "std::map");




	profiler_data_print(profiler);

	return 0;
}