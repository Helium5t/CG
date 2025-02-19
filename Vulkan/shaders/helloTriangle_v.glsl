#version 450

vec2 vertPos[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);


layout(location = 0) out vec3 fragColor; // Channel that allows us to pass the values to frag shader (probably TEXCOORD0 in HLSL)


void main() {
    gl_Position = vec4(vertPos[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}