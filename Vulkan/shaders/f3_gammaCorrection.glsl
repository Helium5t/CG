#version 450

// IN
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 uvMainTex;

// OUT
layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform sampler2D mainTexSampler;


#define GAMMA_FACTOR 0.4545454545454 // 1/2.2

void main() {
    vec4 c = texture(mainTexSampler, uvMainTex);
    c = pow(c, vec4(GAMMA_FACTOR,GAMMA_FACTOR,GAMMA_FACTOR,GAMMA_FACTOR));
    outColor = c;
}