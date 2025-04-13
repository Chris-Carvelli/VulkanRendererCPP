#version 450

layout(binding = 0) uniform UniformBufferObject_Frame {
    mat4 viewProj;
} data_frame;


layout(push_constant) uniform ModelData {
    mat4 model;
    vec3 color;
} data_model;

layout(location = 0) in vec3 inPosition;

void main() {
    vec4 world_pos = data_model.model * vec4(inPosition, 1.0);
    gl_Position = data_frame.viewProj * world_pos;
}