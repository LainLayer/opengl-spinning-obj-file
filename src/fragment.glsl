#version 460 core

layout(location = 0) out vec4 color;

uniform vec4 user_color;
uniform float time;

in float just_z;

void main() {
    float factor = (just_z + 1.0) / 2.0 * 2;
    color = vec4(
        ((sin(time) + 1.0) / 2.0) / factor,
        user_color.yz / factor,
        user_color.w
    );
}