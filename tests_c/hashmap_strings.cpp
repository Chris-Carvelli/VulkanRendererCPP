#include <map>

#include <memory.h>
#include <string>
#include <stdint.h>

#include <stdio.h>

extern "C" {
	#include <cc_logger.h>
	#include <cc_map.h>
	#include <cc_allocator.h>
	#include <cc_profiler.h>

	#include <TMP_map_fixed.h>
}

#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"

int cmp_strings(const void* a, const void* b) {
	return strcmp(*(const char**)a, *(const char**)b);
}

struct cmp_strings_cpp {
	bool operator()(const char* a, const char* b) const {
		return strcmp((const char*)a, (const char*)b) < 0;
	}
};

auto comp = [](const char* a, const char* b) {
	return strcmp((const char*)a, (const char*)b) < 0; 
};

void validate_get(char** values, char** retrieved, const int NUM_ELEMENTS, const char* trial) {
	int num_errors = 0;
	for(int i = 0; i < NUM_ELEMENTS; ++i)
		if(strcmp(values[i], retrieved[i]) != 0) {
			CC_LOG(CC_WARNING, "%3d: expected: %s    retrieved: %s", i, values[i], retrieved[i]);
			++num_errors;
		}
	if (num_errors > 0)
		CC_LOG(CC_ERROR, "%s failed to retrieve correct values (%d errors)", trial, num_errors);
}

void validate_ints(uint32_t* values, uint32_t* retrieved, const int NUM_ELEMENTS, const char* trial) {
	int num_errors = 0;
	for(int i = 0; i < NUM_ELEMENTS; ++i)
		if(values[i] != retrieved[i]) {
			CC_LOG(CC_WARNING, "%8d: expected: %8s    retrieved: %8d", i, values[i], retrieved[i]);
			++num_errors;
		}
	if (num_errors > 0)
		CC_LOG(CC_ERROR, "%s failed to retrieve correct values (%d errors)", trial, num_errors);
}

int main() {
	const int NUM_ELEMENTS = 4096 * 128;
	const int HASHMAP_SIZE = 4096 * 128;

	srand(4243);

	BumpAllocator *allocator_str = allocator_make_bump(MB(64));

	char** keys = (char**)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(const char*));
	uint32_t* values = (uint32_t*)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values_retrieved = (uint32_t*)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(uint32_t));

	const char buf_k[] = "KKKKKKKKKKKKKKKK";
	const char buf_r[] = "RRRRRRRRRRRRRRRR";
	char buf[10];
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i) {
		keys[i] = (char*)allocator_put_str(allocator_str, buf_k);

		_itoa(i, buf, 10);
		for(int j = 0; j < 10; ++j) {
			if(buf[j] == 0)
				break;
			keys[i][j] = buf[j];
		}
	}


	BumpAllocator* allocator = allocator_make_bump(MB(4));
	Profiler* profiler = profiler_shared_create(allocator);

	// cc map
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		values_retrieved[i] = 0;
	Map* a = map_make(HASHMAP_SIZE, sizeof(char*), sizeof(uint32_t), cmp_strings, MB(512));
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]base",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_put(a, &keys[i], &values[i]);
	)

	uint32_t TMP_num_kvps = map_diagnostics_count_kvps(a);
	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]base",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_get(a, &keys[i], &values_retrieved[i]);
	)
	//validate_get(values, values_retrieved, NUM_ELEMENTS, "cc base");
	validate_ints(values, values_retrieved, NUM_ELEMENTS, "cc base");



	//// cc map fixed
	//for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//	values_retrieved[i] = "";
	//MapFixed* c = map_fixed_make(HASHMAP_SIZE, cmp_strings, MB(512));
	//PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]fixed",
	//	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//		map_fixed_put(c, keys[i], i);
	//)
	//	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]fixed",
	//		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//			map_fixed_get(c, keys[i], &values_retrieved[i]);
	//)
	//	validate_get(values, values_retrieved, NUM_ELEMENTS, "cc fixed");

	// std map
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		values_retrieved[i] = 0;
	std::map<const char*, uint32_t, cmp_strings_cpp> b;

	PROFILE(profiler, NULL_SAMPLE_HANDLE, "[put]std",
		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			b[keys[i]] = values[i];
	)
		PROFILE(profiler, NULL_SAMPLE_HANDLE, "[get]std",
			for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
				values_retrieved[i] = b[keys[i]];
	)
		//validate_get(values, values_retrieved, NUM_ELEMENTS, "std::map");
		validate_ints(values, values_retrieved, NUM_ELEMENTS, "cc base");




	profiler_data_print(profiler);

	return 0;
}