#include <cc_profiler.h>
#include <cc_allocator.h>
#include <cc_logger.h>

#include <stdio.h>
int main(void) {
	BumpAllocator* allocator = allocator_make_bump(KB(256));
	Profiler* profiler = profiler_shared_create(allocator);
	int a, b;
	//profiler_highperf_begin(profiler);
	//	for(int i = 0; i < 4096; ++i)
	//	{
	//		profiler_highperf_begin(profiler);
	//			//printf("Hello World\n");
	//				a = b + 2;
	//		res_printf = profiler_highperf_end(profiler);
	//	}
	//res_profiler =  profiler_highperf_end(profiler);

	//CC_LOG(CC_INFO,      "printf    %lld", res_printf);
	//CC_LOG(CC_INFO,      "profiler  %lld", res_profiler);
	//CC_LOG(CC_IMPORTANT, "diff      %lld", res_profiler - res_printf);



	// sampling mode
	uint64_t res_printf_beg, res_printf_end, res_profiler_beg, res_profiler_end;
	res_profiler_beg = profiler_highperf_sample();

	//profiler_highperf_begin(profiler);
	//profiler_highperf_end(profiler);
	res_printf_beg = profiler_highperf_sample();
	res_printf_end = profiler_highperf_sample();

	res_profiler_end =  profiler_highperf_sample();

	uint64_t res_printf = res_printf_end - res_printf_beg;
	uint64_t res_profiler = res_profiler_end - res_profiler_beg;
	CC_LOG(CC_INFO,      "inner    %lld", res_printf);
	CC_LOG(CC_INFO,      "outer    %lld", res_profiler);
	CC_LOG(CC_IMPORTANT, "diff     %lld", res_profiler - res_printf);

	return 0;
}