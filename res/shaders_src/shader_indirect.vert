#version 450

layout(binding = 0) uniform UniformsGlobal {
    mat4 view;
    mat4 proj;
} uniforms_global;

// vertex frequency
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// instance frequency
layout(binding = 1) buffer InstanceData {
    mat4 model_matrices[];
} instance_data;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = uniforms_global.proj * uniforms_global.view * instance_data.model_matrices[gl_InstanceIndex] * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
