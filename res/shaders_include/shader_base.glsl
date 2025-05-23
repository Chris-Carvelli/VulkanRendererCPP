#ifndef SHADER_BASE_H
#define SHADER_BASE_H

const float PI      = 3.14159265;
const float PI_INV  = 0.31830988;
const float TAU     = 6.28318531;
const float TAU_INV = 0.15915492;

float clamp01(float f) { return clamp(f, 0.0, 1.0); }
float clamped_dot(vec3 a, vec3 b) { return max(dot(a, b), 0.0); }


#endif