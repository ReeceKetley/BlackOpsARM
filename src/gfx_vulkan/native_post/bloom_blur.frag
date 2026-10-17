#version 450
// Native bloom_blur_x / bloom_blur_y: RB_SetBlurConstants' symmetric bilinear
// taps along one axis. Taps whose weight is negligible are dropped on the CPU
// and the rest renormalised, so count may be below six.
layout(set = 0, binding = 0) uniform sampler2D source; // linear
layout(set = 0, binding = 8) uniform Params {
    vec4 offsets[2]; // tap distances in UV (xyz, xyz)
    vec4 weights[2]; // matching weights (xyz, xyz)
    vec4 axis;       // xy: unit axis; z: tap count per side
} p;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main() {
    int count = int(p.axis.z);
    vec4 sum = vec4(0.0);
    for (int i = 0; i < 6; ++i) {
        if (i >= count) break;
        float o = p.offsets[i / 3][i % 3];
        float w = p.weights[i / 3][i % 3];
        sum += (texture(source, uv - p.axis.xy * o) + texture(source, uv + p.axis.xy * o)) * w;
    }
    color = sum;
}
