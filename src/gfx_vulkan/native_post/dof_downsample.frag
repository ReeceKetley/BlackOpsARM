#version 450
// Native dof_downsample_nv_intz: each output texel covers a 4x4 scene block.
// Colour: four bilinear taps (16 texels, as the original). Near CoC: the same
// min/max depth reduction over the 16 depth texels, gathered in four fetches.
layout(set = 0, binding = 0) uniform sampler2D scene; // linear
layout(set = 0, binding = 1) uniform sampler2D depth; // point, depth aspect
layout(set = 0, binding = 8) uniform Params {
    vec4 eqScene;      // dofEquationScene
    vec4 eqViewModel;  // dofEquationViewModelAndFarBlur
    vec4 sceneTexel;   // xy: 1 / scene size
} p;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void Reduce(vec4 d, inout float nearest, inout float viewModel) {
    // Viewmodel depths (< eqViewModel.y) are excluded from the scene minimum.
    vec4 vm = clamp(p.eqViewModel.y - d, 0.0, 1.0);
    vec4 sceneDepth = mix(d, vec4(1.0), greaterThan(vm, vec4(0.0)));
    nearest = min(nearest, min(min(sceneDepth.x, sceneDepth.y), min(sceneDepth.z, sceneDepth.w)));
    viewModel = max(viewModel, max(max(vm.x, vm.y), max(vm.z, vm.w)));
}

void main() {
    vec2 t = p.sceneTexel.xy;
    vec3 sum = texture(scene, uv + vec2(-t.x, -t.y)).rgb + texture(scene, uv + vec2(t.x, -t.y)).rgb
             + texture(scene, uv + vec2(-t.x, t.y)).rgb + texture(scene, uv + vec2(t.x, t.y)).rgb;
    float nearest = 1.0, viewModel = 0.0;
    Reduce(textureGather(depth, uv + vec2(-t.x, -t.y)), nearest, viewModel);
    Reduce(textureGather(depth, uv + vec2(t.x, -t.y)), nearest, viewModel);
    Reduce(textureGather(depth, uv + vec2(-t.x, t.y)), nearest, viewModel);
    Reduce(textureGather(depth, uv + vec2(t.x, t.y)), nearest, viewModel);
    float nearCoc = clamp(p.eqScene.x / max(1e-8, 1.0 - nearest) + p.eqScene.z, 0.0, 1.0);
    float viewModelCoc = clamp(p.eqViewModel.x / max(viewModel, 1e-8) + p.eqViewModel.z, 0.0, 1.0);
    color = vec4(sum * 0.25, viewModel > 0.0 ? viewModelCoc : nearCoc);
}
