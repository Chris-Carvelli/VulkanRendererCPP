#include <cc_profiler.h>
#include <cc_allocator.h>
#include <cc_logger.h>

#include <stdio.h>
int main(void) {
	profiler_init();

	for(int i = 0; i < 4096; ++i)
	{
		{
			PROFILE("[macro]outer",
				PROFILE("[macro]inner", {} );
			);
		}

		{
			profiler_sample_begin("[named]outer");
			profiler_sample_begin("[named]inner");
			profiler_sample_end();
			profiler_sample_end();
		}
	}
	profiler_data_print();

	{
		// sampling mode
		uint64_t res_printf_beg, res_printf_end, res_profiler_beg, res_profiler_end;
		res_profiler_beg = profiler_highperf_sample();

		res_printf_beg = profiler_highperf_sample();
		res_printf_end = profiler_highperf_sample();

		res_profiler_end =  profiler_highperf_sample();

		uint64_t res_printf = res_printf_end - res_printf_beg;
		uint64_t res_profiler = res_profiler_end - res_profiler_beg;
		CC_LOG(CC_IMPORTANT, "Highperf sample");
		CC_LOG(CC_INFO,      "inner    %lld", res_printf);
		CC_LOG(CC_INFO,      "outer    %lld", res_profiler);
		CC_LOG(CC_IMPORTANT, "diff     %lld", res_profiler - res_printf);
	}



	return 0;
}