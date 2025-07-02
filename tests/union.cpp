#include <immintrin.h>

#include <glm/glm.hpp>

typedef struct transform_ps {
	__m128 pos_x;
	__m128 pos_y;
	__m128 pos_z;
	__m128 vel_x;
	__m128 vel_y;
	__m128 vel_z;
} transform_ps;

typedef struct transform_glm {
	glm::vec3 pos;
	glm::vec3 vel;
} transform_glm;

void step_ps(transform_ps* transforms, uint32_t n, float delta) {
	transforms[0].pos_x + transforms[0].vel_x;
	for(int i = 0; i < n; ++i)
		_mm_add_ps(transforms[i].pos_x, transforms[i].vel_x * delta);
}

int main() {
	

	float bar = ox4[0];
}