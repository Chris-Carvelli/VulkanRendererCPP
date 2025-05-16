#include "shader_base.glsl"
#include "data_uniform.glsl"

#include "utils.glsl"
#include "pbr_functions.glsl"

layout(binding = 2) uniform sampler2D   tex_albedo;
layout(binding = 3) uniform sampler2D   tex_specular;
layout(binding = 4) uniform sampler2D   tex_normal;
//layout(binding = TODO) uniform sampler2D   tex_emissive;
layout(binding = 5) uniform samplerCube tex_environment;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 albedo = texture(tex_albedo, fragTexCoord);
//	vec4 emissive = texture(tex_emissive, fragTexCoord);
	vec4 params_specular = texture(tex_specular, fragTexCoord);

	vec3 N = sample_normal_map(tex_normal, fragTexCoord, fragNormal, fragTangent);;
	vec3 V = normalize(data_frame.cam_pos - fragPosition);
	vec3 L = normalize(data_frame.light_dir);

	DataMaterial mat;
	mat.albedo = albedo.rgb;
	mat.occlusion = params_specular.r;
	mat.roughness = params_specular.y;
	mat.metalness = params_specular.z;

	outColor = vec4(
		BRDFDirect(L, N, V, mat)
		 + BRDFIndirect(L, N, V, mat, tex_environment)
		// + emissive.rgb
		, 1.0
	);
}