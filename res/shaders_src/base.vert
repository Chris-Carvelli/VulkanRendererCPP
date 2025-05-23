#include "shader_base.glsl"
#include "data_uniform.glsl"

layout(push_constant) uniform ModelData {
    mat4 model;
} data_model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec4 world_pos = data_model.model * vec4(inPosition, 1.0);
    gl_Position = data_frame.proj * data_frame.view * world_pos;

    fragTexCoord = inTexCoord;
}