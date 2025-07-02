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
}

#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"

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

void validate_ints(uint32_t* values, uint32_t* retrieved, uint32_t* hashes, char** keys, const int NUM_ELEMENTS, const char* trial) {
	int num_errors = 0;
	for(int i = 0; i < NUM_ELEMENTS; ++i)
		if(values[i] != retrieved[i]) {
			CC_LOG(CC_WARNING, "%8d: expected: %8d    retrieved: %8d", i, values[i], retrieved[i]);
			if(hashes[values[i]] == hashes[i]) {
				CC_LOG(CC_ERROR, "SAME HASH DETECTED: %16u\t%16u", hashes[values[i]], hashes[i]);
				CC_LOG(CC_ERROR, "                    %s\t%s\n", keys[values[i]], keys[i]);
			}

			++num_errors;
		}
	if (num_errors > 0)
		CC_LOG(CC_ERROR, "%s failed to retrieve correct values (%d errors)", trial, num_errors);
}

int main() {
	const int NUM_ELEMENTS = 4096*22;
	const int HASHMAP_SIZE = 4096*22;

	srand(4242);

	BumpAllocator *allocator_str = allocator_make_bump(MB(128));

	char** keys = (char**)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(const char*));
	uint32_t* values = (uint32_t*)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* hashes = (uint32_t*)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(uint32_t));
	uint32_t* values_retrieved = (uint32_t*)allocator_alloc_n(allocator_str, NUM_ELEMENTS,sizeof(uint32_t));

	const char buf_k[] = "KKKKKKKKKKKKKKKK";
	const char buf_r[] = "RRRRRRRRRRRRRRRR";
	char buf[10];
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i) {
		values[i] = i;
		sprintf(buf, "%d", i);

		//// normal key
		//keys[i] = (char*)allocator_put_str(allocator_str, buf);

		// padded key
		keys[i] = (char*)allocator_put_str(allocator_str, buf_k);
		for(int j = 0; j < 10; ++j) {
			if(buf[j] == 0)
				break;
			keys[i][j] = buf[j];
		}
	}


	BumpAllocator* allocator = allocator_make_bump(MB(4));
	Profiler* profiler = profiler_shared_create(allocator);

	HandleProfilerSample h_put_base = profiler_create_sample_handle_named(profiler, "[put]base");
	HandleProfilerSample h_get_base = profiler_create_sample_handle_named(profiler, "[get]base");
	HandleProfilerSample h_put_std = profiler_create_sample_handle_named(profiler, "[put]std");
	HandleProfilerSample h_get_std = profiler_create_sample_handle_named(profiler, "[get]std");

	// cc map
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		values_retrieved[i] = 0;
	Map* a = map_make(HASHMAP_SIZE, sizeof(uint32_t), MB(512));

	{
		profiler_sample_begin(profiler, h_put_base);
		for (uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			hashes[i] = map_put(a, keys[i], strlen(keys[i]), &values[i]);
		profiler_sample_end(profiler);
	}
	CC_LOG(CC_INFO, "===========");
	//uint32_t TMP_num_kvps = map_diagnostics_count_kvps(a);
	{
		profiler_sample_begin(profiler, h_get_base);
		for (uint32_t i = 0; i < NUM_ELEMENTS; ++i)
			map_get(a, keys[i], strlen(keys[i]), &values_retrieved[i]);
		profiler_sample_end(profiler);
	}
	//validate_get(values, values_retrieved, NUM_ELEMENTS, "cc base");
	validate_ints(values, values_retrieved, hashes, keys, NUM_ELEMENTS, "cc base");

	//// std map
	//for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//	values_retrieved[i] = 0;
	//std::map<const char*, uint32_t, cmp_strings_cpp> b;

	//PROFILE(profiler, h_put_std,
	//	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//		b[keys[i]] = values[i];
	//)
	//	PROFILE(profiler, h_get_std,
	//		for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
	//			values_retrieved[i] = b[keys[i]];
	//)
	//	//validate_get(values, values_retrieved, NUM_ELEMENTS, "std::map");
	//	validate_ints(values, values_retrieved, NUM_ELEMENTS, "cc base");




	profiler_data_print(profiler);

	return 0;
}