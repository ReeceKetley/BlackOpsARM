#version 450
// Native bloom_downsample_ldr + resample_final: scene -> 1/4 size in one pass.
// Each bilinear tap averages one 2x2 scene block (a bloom_mip1 texel), which
// is squared and scaled by 1/8 as bloom_downsample_ldr does; the average of
// the four is the single bilinear resample_final tap into bloom_mip2.
layout(set = 0, binding = 0) uniform sampler2D scene; // linear
layout(set = 0, binding = 8) uniform Params {
    vec4 sceneTexel; // xy: 1 / scene size
} p;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main() {
    vec2 t = p.sceneTexel.xy;
    vec4 a = texture(scene, uv + vec2(-t.x, -t.y));
    vec4 b = texture(scene, uv + vec2(t.x, -t.y));
    vec4 c = texture(scene, uv + vec2(-t.x, t.y));
    vec4 d = texture(scene, uv + vec2(t.x, t.y));
    color = (a * a + b * b + c * c + d * d) * (0.125 * 0.25);
}
