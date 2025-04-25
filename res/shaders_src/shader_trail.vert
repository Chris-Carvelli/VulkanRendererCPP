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

    float f = data_model.radius * ((gl_VertexIndex % 2) * 2 - 1);

//    // FIXME billboarded trail
//    vec3 screen_up = vec3(0.0, 0.0, 1.0);
//    vec4 screen_normal = transpose(data_frame.view) * vec4((inNormal), 0.0);
//    vec4 screen_offset = vec4((cross(screen_normal.xyz, screen_up.xyz)), 0.0);
//    vec4 screen_coords = data_frame.proj * data_frame.view * world_pos;
//    screen_coords += screen_offset * f;
//    gl_Position =  screen_coords;

    // world trail
    world_pos.xyz += cross(inNormal, vec3(0.0, 1.0, 0.0)) * f;
    gl_Position =  data_frame.proj * data_frame.view * world_pos;

    fragPosition = world_pos.xyz;
    fragColor = (gl_VertexIndex % 2) == 0 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}