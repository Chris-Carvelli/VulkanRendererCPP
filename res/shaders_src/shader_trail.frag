#version 450

layout(binding = 1) uniform UniformBufferObject_Material {
    float ambient;
    float diffuse;
    float specular;
    float specular_exp;
} data_material;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0.117, 0.531, 0.894, 1.0);
}
