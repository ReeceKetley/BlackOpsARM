#version 450
// Window output triangle for the SGSR present pass: the same pre-rotated
// position/uv vertices as the passthrough output program.
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;
layout(location = 0) out highp vec4 uv;
void main() {
    uv = vec4(texcoord, 0.0, 0.0);
    gl_Position = position;
}
