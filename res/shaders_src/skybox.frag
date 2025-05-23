#include "shader_base.glsl"
#include "data_uniform.glsl"
#include "utils.glsl"

layout(location = 0) in vec3 fragViewDir;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex_skybox;
//layout(binding = 1) uniform samplerCube tex_skybox;

void main() {
	vec2 uv = uv_spherical_mapping(fragViewDir);
	outColor = texture(tex_skybox, uv);
}