#include <TMP_map_fixed.h>

#include <cc_logger.h>
#include <cc_allocator.h>
#include <cc_hash.h>

static const size_t FIXED_KVP_SIZE = sizeof(uint32_t) * 2 + sizeof(uint32_t*);

//const uint8_t KEY_NOT_FOUND = 0;
//const uint8_t KEY_FOUND     = 1;

static void* NO_FOLLOWER = (void*)-1;
static void* EMPTY_SPOT  = (void*) 0;

typedef struct MapFixed {
	// definition
	size_t max_elements;
	fn_compare_t fn_comparator;

	// runtime
	BumpAllocator* allocator;
	//size_t num_elements;
	//uint32_t* key_hashes;
	uint32_t*  keys;
	uint32_t*  values;
	uint32_t** nexts;
}MapFixed;

MapFixed* map_fixed_make(size_t num_elements, fn_compare_t fn_comparator, size_t max_size) {
	BumpAllocator* allocator = allocator_make_bump(max_size);

	MapFixed* handle = (MapFixed*)allocator_alloc(allocator, sizeof(MapFixed));

	handle->max_elements  = num_elements;
	handle->fn_comparator = fn_comparator;
	handle->allocator     = allocator;

	handle->keys   = (uint32_t*)allocator_alloc_n(allocator, num_elements, sizeof(uint32_t));
	handle->values = (uint32_t*)allocator_alloc_n(allocator, num_elements, sizeof(uint32_t));
	handle->nexts  = (uint32_t**)allocator_alloc_n(allocator, num_elements, sizeof(uint32_t*));

	// since `EMPTY_SPOT` is all 0s, this should work (but it may be not very cross-compatible, need to test)
	memset(handle->nexts, 0, sizeof(uint32_t*) * num_elements);

	return handle;
}

uint8_t map_fixed_put(MapFixed* handle, uint32_t key, uint32_t value) {

	uint32_t hash = SuperFastHash((char*)&key, sizeof(uint32_t));
	hash %= handle->max_elements;

	if (handle->nexts[hash] == EMPTY_SPOT) {
		// no key in this bucket.
		handle->keys[hash] = key;
		handle->values[hash] = value;
		handle->nexts[hash] = NO_FOLLOWER;

		return KEY_NOT_FOUND;
	}

	uint32_t*  ptr_key   = &handle->keys[hash];
	uint32_t*  ptr_value = &handle->values[hash];
	uint32_t** ptr_next  = &handle->nexts[hash];
	// follow hash chain
	while (*ptr_next != NO_FOLLOWER && handle->fn_comparator(&key, ptr_key) != 0)
	{
		ptr_key = *ptr_next;
		ptr_value = ptr_key + 1;
		ptr_next = (uint32_t**)(ptr_key + 2);
	}

	uint8_t ret = KEY_NOT_FOUND;
	if(handle->fn_comparator(&key, ptr_key) != 0) {
		// connect last entry to new entry
		*ptr_next = (uint32_t*)allocator_alloc(handle->allocator, FIXED_KVP_SIZE);

		// update poitners to new KVP entry
		ptr_key = *ptr_next;
		ptr_value = ptr_key + 1;
		ptr_next = (uint32_t**)(ptr_key + 2);

		ret = KEY_FOUND;
	}

	// update new KVP values
	*ptr_key = key;
	*ptr_value = value;
	*ptr_next = NO_FOLLOWER;

	return ret;
}

uint8_t map_fixed_get(MapFixed* handle, uint32_t key, uint32_t* value) {
	uint32_t hash = SuperFastHash((char*)&key, sizeof(uint32_t));
	hash %= handle->max_elements;

	if (handle->nexts[hash] == EMPTY_SPOT)
		return KEY_NOT_FOUND;

	// follow hash chain
	uint32_t*  ptr_key   = &handle->keys[hash];
	uint32_t*  ptr_value = &handle->values[hash];
	uint32_t** ptr_next  = &handle->nexts[hash];
	while (*ptr_next != NO_FOLLOWER && handle->fn_comparator(&key, ptr_key) != 0)
	{
		ptr_key = *ptr_next;
		ptr_value = ptr_key + 1;
		ptr_next = (uint32_t**)(ptr_key + 2);
	}

	if(handle->fn_comparator(&key, ptr_key) != 0)
		return KEY_NOT_FOUND;

	*value = *ptr_value;

	return KEY_FOUND;
}

void map_fixed_destroy(MapFixed* handle);

void map_fixed_diagnostics_print_buckets(MapFixed* handle) {
	const int MAX_BUCKETS = 100;
	uint32_t buckets[MAX_BUCKETS];

	memset(buckets, 0, sizeof(uint32_t) * MAX_BUCKETS);

	int num_elements = 0;
	int num_hops = 0;
	int max_chain_length = 0;
	for(size_t i = 0; i < handle->max_elements; ++i) {
		if(handle->nexts[i] == EMPTY_SPOT) {
			buckets[0]++;
			continue;
		}

		int chain_length = 1;
		++num_hops;++num_hops;
		uint32_t** ptr_next  = &handle->nexts[i];
		while(*ptr_next != NO_FOLLOWER) {
			//ptr_next =( (uint32_t**)(*(char*)ptr_next + sizeof(uint32_t) * 2));
			ptr_next = (uint32_t**)(*ptr_next + 2);

			++chain_length;
			++num_hops;
		}

		if (chain_length > max_chain_length)
			max_chain_length = chain_length;

		//if (chain_length < MAX_BUCKETS)
		buckets[chain_length]++;
		num_elements += chain_length;
		//CC_LOG(CC_INFO, "%3lld\t%3d", i, chain_length); 
	}

	CC_LOG(CC_IMPORTANT, "max_chain_length: %d", max_chain_length); 
	CC_LOG(CC_IMPORTANT, "num elements: %d", num_elements); 
	CC_LOG(CC_IMPORTANT, "histogram"); 
	for(int i = 0; i < MAX_BUCKETS; ++i) {
		if(buckets[i] == 0)
			continue;
		CC_LOG(CC_INFO, "%4d\t%4d", i, buckets[i]);
	}

	CC_LOG(CC_IMPORTANT, "num keys:\t%3d\tnum hops:\t%3d\tfract:\t%f", num_elements, num_hops, (double)num_hops / (double)num_elements);
	CC_LOG(CC_IMPORTANT, "empty bins: %d\t%f", buckets[0], (double)buckets[0]/(double)handle->max_elements);
}