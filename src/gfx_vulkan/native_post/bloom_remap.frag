#version 450
// Native bloom_downsample_convolution + bloom_remap: bloom_mip2 -> bloom_mip3.
// The convolution keeps the original's eight taps (its (-1,-1) corner is not
// sampled, so the weights sum to 15/16); the remap follows the original
// shader with RB_SetBloomConstants' postFxControl0..8.
layout(set = 0, binding = 0) uniform sampler2D source; // linear
layout(set = 0, binding = 8) uniform Params {
    vec4 texel;      // xy: one bloom_mip3 texel in UV
    vec4 control[9]; // postFxControl0..8 (the original's c5..c11, c20, c21)
} p;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

vec4 Power(vec4 x, vec4 e) { return exp2(log2(max(x, vec4(1e-20))) * e); }

void main() {
    vec2 t = p.texel.xy;
    vec4 s = texture(source, uv) * 0.25
        + (texture(source, uv + vec2(-t.x, 0.0)) + texture(source, uv + vec2(0.0, -t.y))
         + texture(source, uv + vec2(t.x, 0.0)) + texture(source, uv + vec2(0.0, t.y))) * 0.125
        + (texture(source, uv + vec2(t.x, -t.y)) + texture(source, uv + vec2(-t.x, t.y))
         + texture(source, uv + t)) * 0.0625;
    vec4 c5 = p.control[0], c6 = p.control[1], c7 = p.control[2], c8 = p.control[3], c9 = p.control[4];
    vec4 c10 = p.control[5], c11 = p.control[6], c20 = p.control[7], c21 = p.control[8];
    float luminance = s.r * c6.z + s.b * c6.z + s.g * c6.w;
    float desaturate = clamp(luminance * c6.x + c6.y, 0.0, 1.0) * c5.w;
    vec4 r0 = vec4(mix(s.rgb, vec3(luminance), desaturate), clamp(dot(s.rgb, c5.xyz), 0.0, 1.0));
    vec4 r1 = Power(clamp(r0 * c8 + c9, 0.0, 1.0), c11) * c21;
    color = Power(clamp(r0 * c7, 0.0, 1.0), c10) * c20 + r1;
}
