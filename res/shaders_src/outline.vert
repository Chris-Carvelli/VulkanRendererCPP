#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform ModelData {
    mat4 model;
} model_data;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

const float outlineSize = 0.03;
void main() {
    vec4 clip_normal = ubo.proj * ubo.view * model_data.model * vec4(inNormal, 0.0);
    gl_Position = ubo.proj * ubo.view * model_data.model * vec4(inPosition, 1.0);
    gl_Position += clip_normal * outlineSize;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
