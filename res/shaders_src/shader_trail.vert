#version 450

#extension GL_ARB_shading_language_include : enable
#include "../shaders_include/data_uniform.glsl"


layout(std430, push_constant) uniform TrailData {
    float radius;
    int offset_dir;
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

    int dir_frame = (data_frame.frame / 1);
    int dir_vertex = (((gl_VertexIndex + dir_frame) % 2) * 2 - 1);
    int dir = dir_vertex;
    float f = data_model.radius * dir * data_model.offset_dir;

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
    fragColor = dir > 0 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;

    
    // flip UVs based on frame
    fragTexCoord.x *= dir_frame == 0 ? 1 : -1;
}