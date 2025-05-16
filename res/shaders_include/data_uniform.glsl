layout(binding = 0) uniform UniformBufferObject_Frame {
    // camera
    mat4 view;
    mat4 proj;
    vec3 cam_pos;

    // lighting
    vec3  light_ambient;
    vec3  light_dir;
    vec3  light_color;
    float light_intensity;

    // app
    int frame;
} data_frame;