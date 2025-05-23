#ifndef UTILS_H
#define UTILS_H
#include "shader_base.glsl"

// Constructs the 3D normal using only XY values (computing implicit Z)
vec3 get_implicit_normal(vec2 normal)
{
	float z = sqrt(1.0 - normal.x * normal.x - normal.y * normal.y);
	return vec3(normal, z);
}

// Sample texture map in tangent space and converts to the same space of the provided normal and tangent 
vec3 sample_normal_map(sampler2D tex_normals, vec2 uvs, vec3 normal, vec3 tangent)
{
	// Build the tangent space base vectors
	normal = normalize(normal);
	vec3 bitangent = normalize(cross(normal, tangent));
	tangent = cross(normal, bitangent);

	// Read normalTexture
	vec2 normal_map = texture(tex_normals, uvs).xy;

	// if DirectX normals, flip green channel
	// THIS SHOULD BE DONE IN IMPORT!
	normal_map.g = 1 - normal_map.g;

	normal_map = normal_map * 2.0 - vec2(1.0);

	// Get implicit Z component
	vec3 normal_tangent_space = get_implicit_normal(normal_map);

	// Create tangent space matrix
	mat3 tangent_matrix = mat3(tangent, bitangent, normal);

	// Return matrix in world space
	return normalize(tangent_matrix * normal_tangent_space);
}

vec2 uv_spherical_mapping(vec3 dir){
//	vec2 uv = vec2(
//		0.5 + atan(fragViewDir.y, fragViewDir.x) / TAU,
//		0.5 + asin(fragViewDir.z) / PI
//	);

	dir = normalize(dir);
	vec2 uv = vec2(
		atan(dir.z, dir.x),
		asin(dir.y)
	);
	uv *= TAU_INV;
	uv += 0.5;

	return uv;
}

#endif