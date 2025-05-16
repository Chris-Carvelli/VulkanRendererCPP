#include "shader_base.glsl"
#include "data_uniform.glsl"

layout(push_constant) uniform DataCamera{
	vec3 pos;
} data_camera;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragViewDir;

void main () {
	// Use always max depth
	gl_Position = vec4(inPosition.xy, 1.0, 1.0);

	vec4 world_pos = inverse(data_frame.proj * data_frame.view) * gl_Position;
	world_pos /= world_pos.w;

	fragViewDir = world_pos.xyz - data_camera.pos;

	// cubemap is left-handed, need to voncert to right handed
	fragViewDir.z *= -1;
}