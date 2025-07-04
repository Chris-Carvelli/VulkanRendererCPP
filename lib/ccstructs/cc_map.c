#include <cc_map.h>

#include <cc_logger.h>
#include <cc_allocator.h>
#include <cc_hash.h>

#include <memory.h> // for memset

// TMP allow dev
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"

const uint64_t KEY_NOT_FOUND = 0;
const uint64_t KEY_FOUND     = 1;

static void* NO_FOLLOWER = (void*)-1;
static void* EMPTY_SPOT  = (void*) 0xfefefefefefefefe;

typedef struct Map {
	// definition
	size_t max_elements;
	size_t size_value;

	// runtime
	BumpAllocator* allocator;
	uint64_t* hashes;
	void*     values;
	void**    nexts;
} Map;

Map* map_make(BumpAllocator* allocator, size_t num_elements, size_t size_value) {
	Map* handle = (Map*)allocator_alloc(allocator, sizeof(Map));

	handle->max_elements  = num_elements;
	handle->size_value    = size_value;
	handle->allocator     = allocator;

	handle->hashes = (uint64_t*)allocator_alloc_n(allocator, num_elements, sizeof(uint64_t));
	handle->values = (void*)allocator_alloc_n(allocator, num_elements, size_value);
	handle->nexts  = (void**)allocator_alloc_n(allocator, num_elements, sizeof(void*));

	// since `EMPTY_SPOT` is all 0s, this should work (but it may be not very cross-compatible, need to test)
	memset(handle->nexts, 0xfe, num_elements * sizeof(void*));

	// DEBUG fill everything with recognizable bit pattern
	memset(handle->hashes, 0xaa, num_elements * sizeof(uint64_t));
	memset(handle->values, 0xbb, num_elements * size_value);

	return handle;
}

uint64_t map_put(Map* handle, const void* key, size_t key_size, const void* value) {
	uint64_t hash = CaseyHash((const char *)key, key_size);
	uint64_t slot = hash % handle->max_elements;

	//CC_LOG(CC_INFO, "%d", hash);

	size_t size_kvp     = sizeof(uint64_t) + handle->size_value + sizeof(void*);
	size_t offset_value = sizeof(uint64_t);
	size_t offset_next  = sizeof(uint64_t) + handle->size_value;

	uint64_t* ptr_key   = &handle->hashes[slot];
	void*     ptr_value = handle->values + handle->size_value * slot;
	void**    ptr_next  = &handle->nexts[slot];

	if (*ptr_next == EMPTY_SPOT) {
		// no key in this bucket
		handle->hashes[slot] = hash;
		memcpy(ptr_value, value, handle->size_value);
		handle->nexts[slot] = NO_FOLLOWER;

		return KEY_NOT_FOUND;
	}

	while (*ptr_next != NO_FOLLOWER && *ptr_key != hash)
	{
		ptr_key   = *ptr_next;
		ptr_value = (void*)ptr_key + offset_value;
		ptr_next  = (void*)ptr_key + offset_next;
	}

	uint64_t ret = KEY_NOT_FOUND;
	if(*ptr_key != hash) {
		// connect last entry to new entry
		*ptr_next = (void*)allocator_alloc(handle->allocator, size_kvp);
	
		// update poitners to new KVP entry
		ptr_key   = *ptr_next;
		ptr_value = (void*)ptr_key + offset_value;
		ptr_next  = (void*)ptr_key + offset_next;
	
		ret = KEY_FOUND;
	}
	else {
	}

	// update new KVP values
	*ptr_key = hash;
	memcpy(ptr_value, value, handle->size_value);
	*ptr_next = NO_FOLLOWER;

	return ret;
}


// TODO must be fixed after figuring out what's wrong with pointer jumps
uint64_t map_get(Map* handle, const void* key, size_t key_size, void* value) {
	uint64_t hash = CaseyHash((const char *)key, key_size);
	uint64_t slot = hash % handle->max_elements;

	//CC_LOG(CC_INFO, "%d", hash);

	void** ptr_next  = &handle->nexts[slot];

	if (*ptr_next == EMPTY_SPOT)
		return KEY_NOT_FOUND;

	size_t offset_value = sizeof(uint64_t);
	size_t offset_next = sizeof(uint64_t) + handle->size_value;
	uint64_t* ptr_key = &handle->hashes[slot];
	void*  ptr_value  = handle->values + handle->size_value * slot;
	while (*ptr_next != NO_FOLLOWER && *ptr_key != hash)
	{
		ptr_key   = *ptr_next;
		ptr_value = (void*)ptr_key + offset_value;
		ptr_next  = (void*)ptr_key + offset_next;
	}

	if(*ptr_key != hash)
		return KEY_NOT_FOUND;
	
	memcpy(value, ptr_value, handle->size_value);

	return KEY_FOUND;
}

uint64_t map_del(Map* handle, const void* key, size_t key_size, void* value) {
	CC_LOG(CC_WARNING, "key removal not implemented yet");
	return KEY_NOT_FOUND; // TODO

	//uint64_t hash = SuperFastHash((const char *)key, handle->size_key);
	//hash %= handle->max_elements;

	//void** ptr_next  = handle->nexts + hash;

	//if(ptr_next == EMPTY_SPOT)
	//	return KEY_NOT_FOUND;

	//size_t offset_value = handle->size_value;
	//size_t offset_next = handle->size_key + handle->size_value;

	//void* ptr_key  = handle->keys + handle->size_key * hash;
	//void* ptr_value  = handle->values + handle->size_value * hash;

	//while (*ptr_next != NO_FOLLOWER && handle->fn_comparator(key, ptr_key) != 0) {
	//	ptr_key   = *ptr_next;
	//	ptr_value = ptr_key + offset_value;
	//	ptr_next  = (void*)ptr_key + offset_next;
	//}

	//if(handle->fn_comparator(key, ptr_key) != 0)
	//	return KEY_NOT_FOUND;

	//// TODO stich everything tigheter without gaps?
	//// return last value in map
	//memcpy(value, ptr_value, handle->size_value);

	//if (*ptr_next != NO_FOLLOWER)
	//handle->nexts[hash] = EMPTY_SPOT;

	//return KEY_FOUND;
}

void map_destroy(Map* handle) {
	// we acltually create and own our own allocator. destroy it
	allocator_free_bump(handle->allocator);
}

uint64_t map_diagnostics_count_kvps(Map* handle) {
	uint64_t ret = 0;

	for(size_t i = 0; i < handle->max_elements; ++i)
		if(handle->nexts[i] != EMPTY_SPOT)
			++ret;

	void** next = handle->nexts + handle->max_elements;
	// offset actual next "field" in the (key, value, next) "tuple"
	next = (void*)next + sizeof(uint64_t) + handle->size_value;

	size_t size_kvp = sizeof(uint64_t) + handle->size_value + sizeof(void*);

	while(*next != EMPTY_SPOT) {
		next = (void*)next + size_kvp;
		++ret;
	}

	return ret;
}

void map_diagnostics_print_buckets(Map* handle) {
	size_t size_kvp = sizeof(uint64_t) + handle->size_value + sizeof(void*);
	size_t offset_next = sizeof(uint64_t) + handle->size_value; 
	
	const int MAX_BUCKETS = 100;
	uint64_t buckets[MAX_BUCKETS];

	memset(buckets, 0, sizeof(uint64_t) * MAX_BUCKETS);

	int num_elements = 0;
	int num_hops = 0;
	int max_chain_length = 0;
	for(size_t i = 0; i < handle->max_elements; ++i) {
		void*  kvp  = (void*)&handle->hashes[i];
		void** next = &handle->nexts[i];

		if(*next == EMPTY_SPOT) {
			buckets[0]++;
			continue;
		}

		int chain_length = 1;
		num_hops += 2;
		while(*next != NO_FOLLOWER) {
			kvp  = *next;
			next = kvp + offset_next;
			++chain_length;
			++num_hops;
		}

		if (chain_length > max_chain_length)
			max_chain_length = chain_length;

		buckets[chain_length]++;
		num_elements += chain_length;
	}

	CC_LOG(CC_IMPORTANT, "max_chain_length: %d", max_chain_length); 
	CC_LOG(CC_IMPORTANT, "num elements: %d", num_elements); 
	CC_LOG(CC_IMPORTANT, "histogram"); 
	CC_LOG(CC_INFO, "%4d\t%4d", 0, buckets[0]);
	for(int i = 1; i < MAX_BUCKETS; ++i) {
		if(buckets[i] == 0)
			continue;
		CC_LOG(CC_INFO, "%4d\t%4d", i, buckets[i]);
	}

	CC_LOG(CC_IMPORTANT, "num keys:\t%3d\tnum hops:\t%3d\tfract:\t%f", num_elements, num_hops, (double)num_hops / (double)num_elements);
	CC_LOG(CC_IMPORTANT, "empty bins: %d\t%f", buckets[0], (double)buckets[0]/(double)handle->max_elements);
}
