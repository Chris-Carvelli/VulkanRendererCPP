#include "shader_base.glsl"
#include "data_uniform.glsl"

#include "utils.glsl"
#include "pbr_functions.glsl"

const int TEX_ALBEDO   = 0;
const int TEX_SPECULAR = 1;
const int TEX_NORMAL   = 2;
//const int TEX_EMISSIVE = 3;

layout(binding = 2) uniform sampler2D textures[3];

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 albedo = texture(textures[TEX_ALBEDO], fragTexCoord);
//	vec4 emissive = texture(textures[TEX_EMISSIVE], fragTexCoord);
	vec4 params_specular = texture(textures[TEX_SPECULAR], fragTexCoord);

	vec3 N = sample_normal_map(textures[TEX_NORMAL], fragTexCoord, fragNormal, fragTangent);;
	vec3 V = normalize(data_frame.view[3].xyz - fragPosition);
	vec3 L = normalize(data_frame.light_dir);

	DataMaterial mat;
	mat.albedo = albedo.rgb;
	mat.occlusion = params_specular.r;
	mat.roughness = params_specular.y;
	mat.metalness = params_specular.z;

	outColor = vec4(
		BRDFDirect(L, N, V, mat)
		 + BRDFIndirect(L, N, V, mat)
		// + emissive.rgb
		, 1.0
	);
}
