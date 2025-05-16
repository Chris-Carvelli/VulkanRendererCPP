#include "shader_base.glsl"
#include "data_uniform.glsl"

layout(location = 0) in vec3 fragViewDir;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube tex_skybox;

void main() {
//	outColor = vec4(fragViewDir, 1.0f);
	outColor = textureLod(tex_skybox, fragViewDir, 0);
}