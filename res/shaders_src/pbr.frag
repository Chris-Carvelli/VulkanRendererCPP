#include "shader_base.glsl"
#include "data_uniform.glsl"

#include "utils.glsl"
#include "pbr_functions.glsl"

layout(binding = 2) uniform sampler2D   tex_albedo;
layout(binding = 3) uniform sampler2D   tex_specular;
layout(binding = 4) uniform sampler2D   tex_normal;
//layout(binding = TODO) uniform sampler2D   tex_emissive;

//layout(binding = 5) uniform samplerCube tex_environment;
layout(binding = 5) uniform sampler2D tex_environment;

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

	vec3 N = sample_normal_map(tex_normal, fragTexCoord, normalize(fragNormal), normalize(fragTangent));
//	vec3 V = normalize(data_frame.cam_pos - fragPosition);
	vec3 V = normalize(get_camera_position(data_frame.view) - fragPosition);
	vec3 L = normalize(data_frame.light_dir);
//	vec3 L = normalize(data_frame.light_dir - fragPosition);

	vec3 H = normalize(L + V);

	DataMaterial mat;
	mat.albedo = albedo.rgb;
	mat.occlusion = params_specular.x;
	mat.roughness = params_specular.y;
	mat.metalness = params_specular.z;

	vec3 final_color = vec3(0.0);
	// debug total light
	if((data_frame.DEBUG_light_components & DEBUG_LIGHT_COMPONENT_DIRECT) != 0)
		final_color += BRDFDirect(L, N, V, mat);

	if((data_frame.DEBUG_light_components & DEBUG_LIGHT_COMPONENT_INDIRECT) != 0)
		final_color += BRDFIndirect(L, N, V, mat, tex_environment);

//	outColor = vec4(final_color, 1.0);
	outColor = vec4(N / 2.0 + 0.5, 1.0);
}
