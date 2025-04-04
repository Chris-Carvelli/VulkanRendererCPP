#version 450

layout(binding = 0) uniform sampler2D tex_color;
layout(binding = 1) uniform sampler2D tex_depth;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// TMP hardcoded near and far plane values
const float zNear = 0.1;
const float zFar = 20.0;

float GetLinearDepth(vec2 coords) {
    float d = texture(tex_color, fragTexCoord).r;

    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main() {
    ivec2 tex_size = textureSize(tex_depth, 0);
    float dx = 0.5/tex_size.x;
    float dy = 0.5/tex_size.y;

    vec3 color = texture(tex_color, fragTexCoord).rgb;
    float d  = GetLinearDepth(fragTexCoord).x;
	float du = GetLinearDepth(fragTexCoord+vec2(0.0, dy)).x;
	float dd = GetLinearDepth(fragTexCoord+vec2(0.0, -dy)).x;
	float dr = GetLinearDepth(fragTexCoord+vec2(dx, 0.0)).x;
	float dl = GetLinearDepth(fragTexCoord+vec2(-dx, 0.0)).x;


	float t = abs(abs(d)-abs(du)) + abs(abs(d)-abs(dd)) + abs(abs(d)-abs(dl)) + abs(abs(d)-abs(dr));
    color = mix(color, vec3(1, 0, 0), t);

    outColor = vec4(1, 0, 0, 1.0);
}
