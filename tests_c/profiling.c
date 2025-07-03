#include <cc_profiler.h>
#include <cc_allocator.h>
#include <cc_map.h>
#include <cc_logger.h>

#include <stdint.h>

const uint32_t NUM_ELEMENTS = 4096;

int cmp_ints(const void* a, const void* b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
}

int main(void) {
	BumpAllocator* allocator = allocator_make_bump(KB(64));
	Profiler*      profiler  = profiler_shared_create(allocator);
	Map* map = map_make(1024, sizeof(uint32_t), MB(128));

	for(int i = 0; i < NUM_ELEMENTS; ++i)
		PROFILE(profiler, "put", map_put(map, &i, sizeof(uint32_t), &i);)

	for(int i = 0; i < NUM_ELEMENTS; ++i)
	{
		uint32_t ret;
		PROFILE(profiler, "get", map_get(map, &i, sizeof(uint32_t), &ret);)
		CC_ASSERT(ret == i, "different values");

	}

	profiler_data_print(profiler);
	return 0;
}
