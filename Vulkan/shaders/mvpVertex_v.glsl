#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 uvMainTex;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;


void main(){
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(inPosition, 1.0);
    outColor = inColor;
    uvMainTex = uv;
}
