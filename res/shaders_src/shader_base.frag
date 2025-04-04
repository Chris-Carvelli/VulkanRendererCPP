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

layout(binding = 1) uniform UniformBufferObject_Material {
    float ambient;
    float diffuse;
    float specular;
    float specular_exp;
} data_material;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(data_frame.view[3].xyz - fragPosition.xyz);
    vec3 L = normalize(data_frame.light_dir);
    vec3 H = normalize(L + V);

    vec3 ambient = albedo * data_material.ambient * data_frame.light_ambient;
    vec3 diffuse = albedo * data_material.diffuse * max(dot(N, L), 0.0);
    vec3 specular = data_frame.light_color * data_material.specular * pow(max(dot(H, N), 0.0), data_material.specular_exp);

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
