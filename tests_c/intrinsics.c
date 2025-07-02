#include <time.h>
#include <windows.h>
#include <intrin.h>

#include <stdint.h>
#include <stdio.h>

#include <immintrin.h>

#include <math.h>

inline void do_work() {
	puts("hello world");
}

typedef struct transform {
	__m128 pos_x;
	__m128 pos_y;
	__m128 pos_z;
	__m128 rot_x;
	__m128 rot_y;
	__m128 rot_z;
	__m128 scl_x;
	__m128 scl_y;
	__m128 scl_z;
} transform;

void mul_normal(float * restrict a, float* restrict b, int n, float c) {
	for(int i = 0; i < n; ++i)
		a[i] = b[i] * c;
}

void mul_ps(__m128 * restrict a, __m128* restrict b, int n, float c) {
	for(int i = 0; i < n; ++i)
		a[i] = b[i] * c;
}

const size_t COUNT = 4096 * 1024;
const size_t COUNT_MM = COUNT / 4;
const size_t MAX_VAL = 10;
int main() {
	srand(424242);

	LARGE_INTEGER freq;
	float delta = 1.0f / 60.0f;
	QueryPerformanceFrequency(&freq);
	__m128* intrinsics_src = (__m128*)calloc(COUNT_MM, sizeof(__m128));
	__m128* intrinsics_dst = (__m128*)calloc(COUNT_MM, sizeof(__m128));
	float* normal_src = (float*)calloc(COUNT, sizeof(float));
	float* normal_dst = (float*)calloc(COUNT, sizeof(float));


	int idx = 0;
	for(int i = 0; i < COUNT_MM; ++i)
	{
		normal_src[idx + 0] = (rand() % MAX_VAL * 10) / (float)MAX_VAL;
		normal_src[idx + 1] = (rand() % MAX_VAL * 10) / (float)MAX_VAL;
		normal_src[idx + 2] = (rand() % MAX_VAL * 10) / (float)MAX_VAL;
		normal_src[idx + 3] = (rand() % MAX_VAL * 10) / (float)MAX_VAL;
		intrinsics_src[i] = _mm_set_ps(
			normal_src[idx + 3],
			normal_src[idx + 2],
			normal_src[idx + 1],
			normal_src[idx + 0]
		);

		idx += 4;
	}

	{
		unsigned long long time_beg, time_end;
		unsigned int core_beg, core_end;
		time_beg = __rdtscp(&core_beg);
		for(int i = 0; i < COUNT; ++i)
			normal_src[i] = normal_src[i] * delta;
		//mul_normal(normal_dst, normal_src, COUNT, delta);
		time_end = __rdtscp(&core_end);
		unsigned long long ticks = time_end - time_beg;
		printf("normal:     %f\t%I64d ticks elapsed\n", (double)ticks / (double)freq.QuadPart, ticks);
	}

	{
		unsigned long long time_beg, time_end;
		unsigned int core_beg, core_end;
		time_beg = __rdtscp(&core_beg);
		for(int i = 0; i < COUNT_MM; ++i)
			intrinsics_src[i] = intrinsics_src[i] * delta;
		time_end = __rdtscp(&core_end);
		unsigned long long ticks = time_end - time_beg;
		printf("intrinsics: %f\t%I64d ticks elapsed\n", (double)ticks / (double)freq.QuadPart, ticks);
	}

	// check
	int src_errors = 0, dst_errors = 0;
	idx = 0;
	const float epsilon = 0.00001f;
	for(int i = 0; i < COUNT_MM; ++i)
	{
		for(int j = 0; j < 4; ++j)
		{
			if(fabs(normal_src[idx + j] - intrinsics_src[i][j]) > epsilon)
			{
				printf("src error %6d:\t%f != %f\n", idx + j, normal_src[idx + j], intrinsics_src[i][j]);
				++src_errors;
			}
			if(fabs(normal_dst[idx + j] - intrinsics_dst[i][j]) > epsilon)
			{
				printf("dst error %6d:\t%f != %f\n", idx + j, normal_dst[idx + j], intrinsics_dst[i][j]);
				++dst_errors;
			}
		}
		idx += 4;
	}

	printf("src errors: %d\n", src_errors);
	printf("dst errors: %d\n", dst_errors);
	free(intrinsics_src);
	free(normal_src);

	return 0;
}