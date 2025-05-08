#version 450

// IN
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 uvMainTex;

// OUT
layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform sampler2D mainTexSampler;


void main() {
    outColor = texture(mainTexSampler, uvMainTex*2.0);
}