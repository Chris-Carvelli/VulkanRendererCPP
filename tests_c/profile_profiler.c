#include <cc_profiler.h>
#include <cc_allocator.h>

#include <stdio.h>

int main(void) {
	BumpAllocator* allocator = allocator_make_bump(KB(256));
	Profiler* profiler = profiler_shared_create(allocator);

	profiler_sample_begin(profiler, "profiler");
		profiler_sample_begin(profiler, "printf");
			printf("Hello World\n");
		profiler_sample_end(profiler);
	profiler_sample_end(profiler);

	profiler_data_print(profiler);

	return 0;
}