#version 450
// Native fused post: postfx_dof_nv_intz blend, then bloom_apply_ldr film curve,
// then postfx_color vision-set colour correction, written once at full res.
// Each stage ports the original pixel shader's arithmetic; only the order of
// DoF relative to bloom/colour differs (DoF now blends before tone mapping).
layout(set = 0, binding = 0) uniform sampler2D scene;  // resolved scene, linear
layout(set = 0, binding = 1) uniform sampler2D depth;  // float Z (depth aspect), point
layout(set = 0, binding = 2) uniform sampler2D medium; // post effect 1: medium blur + near CoC
layout(set = 0, binding = 3) uniform sampler2D large;  // post effect 0: large blur
layout(set = 0, binding = 4) uniform sampler2D bloom;  // final bloom mip
layout(set = 0, binding = 8) uniform Params {
    vec4 texel;        // xy: 1 / scene size, z: depth of field enabled
    vec4 eqScene;      // dofEquationScene
    vec4 eqViewModel;  // dofEquationViewModelAndFarBlur
    vec4 lerpScale;    // dofLerpScale
    vec4 lerpBias;     // dofLerpBias
    vec4 film[3];      // film curve postFxControl0..2
    vec4 grade[11];    // colour correction postFxControl0..A
} p;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

vec3 Matrix(vec3 c, int first) {
    vec4 v = vec4(c, 1.0);
    return clamp(vec3(dot(v, p.grade[first]), dot(v, p.grade[first + 1]), dot(v, p.grade[first + 2])), 0.0, 1.0);
}

void main() {
    vec4 center = texture(scene, uv);
    vec3 c = center.rgb;
    if (p.texel.z > 0.5) {
        vec2 t = p.texel.xy;
        vec3 ring = texture(scene, uv + t * vec2(0.5, -1.5)).rgb + texture(scene, uv + t * vec2(-1.5, -0.5)).rgb
                  + texture(scene, uv + t * vec2(-0.5, 1.5)).rgb + texture(scene, uv + t * vec2(1.5, 0.5)).rgb;
        vec3 small = ring * 0.235294119 + center.rgb * 0.0588235296;
        float d = texture(depth, uv).r;
        vec4 mid = texture(medium, uv);
        vec3 wide = texture(large, uv).rgb;
        float coc = mid.a;
        if (d != 1.0) {
            float far = clamp(p.eqScene.y / max(1e-8, 1.0 - d) + p.eqScene.w, 0.0, 1.0);
            float viewModel = p.eqViewModel.y - d;
            coc = viewModel <= 0.0 ? max(far * p.eqViewModel.w, mid.a)
                                   : clamp(p.eqViewModel.x / viewModel + p.eqViewModel.z, 0.0, 1.0);
        }
        vec4 w = clamp(coc * p.lerpScale + p.lerpBias, 0.0, 1.0);
        float smallWeight = min(1.0 - w.x, w.y), mediumWeight = min(1.0 - w.y, w.z);
        c = center.rgb * w.x + small * smallWeight + mid.rgb * mediumWeight + wide * w.w;
    }
    // bloom_apply_ldr: x = bloom*64 + scene^2*8, rational film curve.
    vec3 x = texture(bloom, uv).rgb * 64.0 + c * c * 8.0;
    vec3 numerator = x * (x * p.film[1].x + p.film[1].y) + p.film[1].z;
    vec3 denominator = x * (x * p.film[0].x + p.film[0].y) + p.film[0].z;
    c = clamp((numerator / denominator + p.film[2].x) * p.film[2].y, 0.0, 1.0);
    // postfx_color: shadow/mid/highlight matrices blended by luminance bands.
    float luminance = dot(c, vec3(0.25, 0.5, 0.25));
    float inverse = 1.0 - luminance;
    vec2 band = clamp(luminance * p.grade[10].xz + p.grade[10].yw, 0.0, 1.0);
    vec3 shadows = Matrix(mix(c, vec3(inverse), p.grade[9].x), 0);
    vec3 mids = Matrix(mix(c, vec3(inverse), p.grade[9].y), 3);
    vec3 highlights = Matrix(mix(c, vec3(inverse), p.grade[9].z), 6);
    vec3 graded = mix(shadows, mids, band.x);
    color = vec4(graded + band.y * (highlights - graded), center.a);
}
