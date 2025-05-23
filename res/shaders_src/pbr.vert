#include "shader_base.glsl"
#include "data_uniform.glsl"

layout(push_constant) uniform ModelData {
    mat4 model;
} data_model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec2 fragTexCoord;

void main() {
    vec4 world_pos     = data_model.model * vec4(inPosition, 1.0);
    vec4 world_normal  = data_model.model * vec4(inNormal,   0.0);
    vec4 world_tangent = data_model.model * vec4(inTangent,  0.0);

    gl_Position = data_frame.proj * data_frame.view * world_pos;

    fragPosition = world_pos.xyz;
    fragColor    = inColor;
    fragNormal   = world_normal.xyz;
    fragTangent  = world_tangent.xyz;
    fragTexCoord = inTexCoord;
}