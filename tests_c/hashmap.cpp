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
	profiler_init();

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

	BumpAllocator* allocator = allocator_make_bump(MB(512));

	Map* a = map_make(allocator, HASHMAP_SIZE, sizeof(uint32_t));
	std::map<uint32_t, uint32_t> b;
	uint32_t val;
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE("[put]base", map_put(a, &keys[i], sizeof(uint32_t), &val); );
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE("[get]base", map_get(a, &keys[i], sizeof(uint32_t), &values_retrieved[i]); );
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE("[put]std", b[keys[i]] = i; );
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE("[get]std", values_retrieved[i] = b[keys[i]]; );


	profiler_data_print();

	allocator_free_bump(allocator);
}