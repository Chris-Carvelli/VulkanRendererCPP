#version 450

layout(push_constant) uniform ModelData {
    mat4 model;
    vec3 color;
} data_model;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(data_model.color, 1.0);
}

//0.957, 0.488, 0.0