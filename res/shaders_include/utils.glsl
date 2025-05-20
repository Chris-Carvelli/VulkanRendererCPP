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
	vec2 normal_map = texture(tex_normals, uvs).xy * 2 - vec2(1);

	// Get implicit Z component
	vec3 normal_tangent_space = get_implicit_normal(normal_map);

	// Create tangent space matrix
	mat3 tangent_matrix = mat3(tangent, bitangent, normal);

	// Return matrix in world space
	return normalize(tangent_matrix * normal_tangent_space);
}
