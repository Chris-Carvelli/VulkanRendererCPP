#include <cc_logger.h>
#include <cc_hash.h>
#include <cc_allocator.h>
#include <cc_profiler.h>

#include <stdlib.h>


int hash_comparator(const void* a, const void* b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
};

const int NUM_KEYS = 4096 * 4096;
const int NUM_ALGO = 2;

uint64_t (*fn_hash[])(const char * data, size_t len) = {
	Lookup3,
	CaseyHash
};
const char* hash_names[] = {
	"Lookup3",
	"CaseyHash"
};

int main(void) {
	BumpAllocator* allocator = allocator_make_bump(GB(1));

	char** keys = allocator_alloc_n(allocator, sizeof(char**), NUM_KEYS);
	uint64_t** results = allocator_alloc_n(allocator, sizeof(uint64_t*), NUM_ALGO);
	for(int i = 0; i < NUM_ALGO; ++i)
		results[i] = allocator_alloc_n(allocator, sizeof(uint32_t), NUM_KEYS);


	//// trial 0: sequenatial keys
	//char buf[16];
	//for(int i = 0; i < NUM_KEYS; ++i) {
	//	keys[i] = allocator_put_str(allocator, _itoa(i, buf, 10));
	//}

	//// trial 1: mostly-equal keys
	//char buf[16];
	//const char buf_k[] = "KKKKKKKKKKKKKKKK";
	//for(int i = 0; i < NUM_KEYS; ++i) {
	//	keys[i] = allocator_put_str(allocator, buf_k);
	//	_itoa(i, buf, 10);
	//	for(int j = 0; j < 10; ++j) {
	//		if(buf[j] == 0)
	//			break;
	//		keys[i][j] = buf[j];
	//	}
	//}

	// trial 2: mostly-equal keys
	char buf[16];
	const char buf_k[] = "KKKKKKKKKKKKKKKK";
	for(int i = 0; i < NUM_KEYS; ++i) {
		keys[i] = allocator_put_str(allocator, buf_k);
		_itoa(i, buf, 10);
		for(int j = 0; j < 10; ++j) {
			if(buf[j] == 0)
				break;
			keys[i][j] = buf[j];
		}
	}

	Profiler* profiler = profiler_shared_create(allocator);

	CC_LOG(CC_IMPORTANT, "%16s %6s %8s", "Function", "Key", "Hash");
	for(int i = 0; i < NUM_ALGO; ++i) {
		HandleProfilerSample h = profiler_create_sample_handle_named(profiler, hash_names[i]);

		for(int j = 0; j < NUM_KEYS; ++j) {
			uint64_t hash;
			PROFILE(profiler, h, hash = fn_hash[i](keys[j], strlen(keys[j]));)
			
			results[i][j] = hash;
		}
	}

	// check collisions
	CC_LOG(CC_IMPORTANT, "%16s  %6s", "Function", "Collisions");
	for(int i = 0; i < NUM_ALGO; ++i) {
		int collisions = 0;
		qsort(results[i], NUM_KEYS, sizeof(uint32_t), hash_comparator);
		for(int j = 0; j < NUM_KEYS - 1; ++j)
			if (results[i][j] == results[i][j + 1])
				++collisions;
		CC_LOG(CC_INFO, "%16s: %10d %3.12f", hash_names[i], collisions, (double)collisions / NUM_KEYS);

	}

	profiler_data_print(profiler);
	return 0;
};