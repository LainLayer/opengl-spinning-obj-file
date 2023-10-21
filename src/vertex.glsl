#version 460 core

layout(location = 0) in vec4 position;

uniform mat4 rotationMatrix;

out float just_z;

void main() {
    vec4 npos = position * rotationMatrix;
    just_z = npos.z;

    gl_Position = npos;
}