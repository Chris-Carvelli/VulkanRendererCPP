const uint DEBUG_LIGHT_COMPONENT_DIRECT   = 1;
const uint DEBUG_LIGHT_COMPONENT_INDIRECT = 2;
const uint DEBUG_LIGHT_COMPONENT_AMBIENT  = 4;

layout(binding = 0) uniform UniformBufferObject_Frame {
    // camera
    mat4 view;
    mat4 proj;
    vec3 cam_pos;

    // lighting
    vec3  light_ambient;
    vec3  light_dir;
    uint   DEBUG_light_components;

    vec3  light_color;
    float light_intensity;


    // app
    int frame;
} data_frame;
