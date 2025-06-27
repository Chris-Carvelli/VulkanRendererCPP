#include <stdint.h>
#include <stdio.h>
#include <uchar.h>
#include <memory.h>
#include <stdlib.h>

#include <cc_hash.h>
#include <cc_logger.h>
#include <cc_map.h>
#include <cc_allocator.h>

int cmp_integers(const void* a, const void* b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
}
int main() {
	////******************************************************
	//// test hash function balance
	//// *****************************************************
	//const uint32_t NUM_TRIALS = 4096;
	//const uint32_t NUM_BUCKETS = 4096;

	//uint32_t buckets[NUM_BUCKETS];

	//memset(buckets, 0, sizeof(buckets));
	//for(int i = 0; i < NUM_TRIALS; ++i) {
	//	uint32_t key = rand();
	//	uint32_t hash = SuperFastHash(&key, sizeof(uint32_t));
	//	uint32_t idx_bucket = hash % NUM_BUCKETS;
	//	buckets[idx_bucket]++;
	//}

	//
	//uint32_t buckets_hist[NUM_BUCKETS];
	//memset(buckets_hist, 0, sizeof(buckets_hist));
	//for(int i = 0; i < NUM_BUCKETS; ++i) {
	//	/*if(buckets[i] == 0)
	//		continue;*/
	//	//CC_LOG(LOG, "%4d\t%4d", i, buckets[i]);
	//	buckets_hist[buckets[i]]++;
	//}

	//CC_LOG(IMPORTANT, "hash distribution histogram");
	//CC_LOG(LOG, "%4d\t%4d", 0, buckets_hist[0]);
	//for(int i = 1; i < NUM_BUCKETS; ++i) {
	//	if (buckets_hist[i] == 0)
	//		continue;
	//	CC_LOG(LOG, "%4d\t%4d", i, buckets_hist[i]);
	//}

	// ******************************************************
	// test generic hashmap bucket spread
	// *****************************************************
	srand(4243);

	const int NUM_ELEMENTS = 4096;
	Map* a = map_make(NUM_ELEMENTS, sizeof(uint32_t), sizeof(uint32_t), cmp_integers, MB(512));

	uint32_t keys[NUM_ELEMENTS];
	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		keys[i] = rand() % UINT32_MAX;

	for(uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		map_put(a, &keys[i], &i);

	map_diagnostics_print_buckets(a);


	////******************************************************
	//// test uint32_t hashmap bucket spread
	//// *****************************************************
	//srand(12);
	//const int NUM_ELEMENTS = 4096;
	//MapFixed* a = map_fixed_make(NUM_ELEMENTS, cmp_integers, MB(512));

	//for(uint32_t i = 0; i < NUM_ELEMENTS; ++i) {
	//	//uint32_t key = rand() % UINT32_MAX;
	//	uint32_t key = i;
	//	map_fixed_put(a, key, i);
	//}

	//map_fixed_diagnostics_print_buckets(a);

	return 0;
}