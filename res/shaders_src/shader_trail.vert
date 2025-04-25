#version 450

layout(binding = 0) uniform UniformBufferObject_Frame {
    mat4 view;
    mat4 proj;

    // lighting
    vec3  light_ambient;
    vec3  light_dir;
    vec3  light_color;
    float light_intensity;
} data_frame;


layout(std430, push_constant) uniform TrailData {
    float radius;
    float viewport_half_width;
    float viewport_half_height;
} data_model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

void main() {
    vec4 world_pos = vec4(
        inPosition,
        1.0f
    );

    mat4 billboarded_view = data_frame.view;
    billboarded_view[0][0] = 1;
    billboarded_view[0][1] = 0;
    billboarded_view[0][2] = 0;
    billboarded_view[1][0] = 0;
    billboarded_view[1][1] = 1;
    billboarded_view[1][2] = 0;
    billboarded_view[2][0] = 0;
    billboarded_view[2][1] = 0;
    billboarded_view[2][2] = 1;

    float f = data_model.radius * ((gl_VertexIndex % 2) * 2 - 1);
    world_pos.y += f;

    vec4 screen_coords = data_frame.view * world_pos;
    
    gl_Position = data_frame.proj * screen_coords;

    fragPosition = world_pos.xyz;
    fragColor = inColor;
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}