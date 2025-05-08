#version 450

// IN
layout(location = 0) in vec3 inColor;

// OUT
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor,1.0);
}