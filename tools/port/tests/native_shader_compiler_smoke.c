#include <vkd3d_shader.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char *message) {
    if (!condition) { fprintf(stderr, "FAIL %s\n", message); abort(); }
}
static int compile(const char *source, const char *profile, struct vkd3d_shader_code *code, char **messages) {
    struct vkd3d_shader_hlsl_source_info hlsl = {0};
    hlsl.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
    hlsl.entry_point = "main"; hlsl.profile = profile;
    struct vkd3d_shader_compile_info info = {0};
    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = &hlsl;
    info.source.code = source; info.source.size = strlen(source);
    info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    info.target_type = VKD3D_SHADER_TARGET_D3D_BYTECODE;
    info.log_level = VKD3D_SHADER_LOG_INFO;
    info.source_name = "bo1-native-compiler-fixture.hlsl";
    return vkd3d_shader_compile(&info, code, messages);
}
static void valid(const char *source, const char *profile, uint32_t version) {
    struct vkd3d_shader_code code = {0}; char *messages = NULL;
    int result = compile(source, profile, &code, &messages);
    if (messages) fputs(messages, stderr);
    require(result >= 0 && code.code && code.size >= 8 && !(code.size % 4), "real compiled bytecode");
    const uint32_t *words = code.code;
    const size_t count = code.size / 4;
    require(words[0] == version && words[count - 1] == 0xffffu, "SM3 stage header and END");
    unsigned tables = 0;
    for (size_t at = 1; at < count - 1;) {
        unsigned opcode = words[at] & 0xffffu;
        size_t length = opcode == 0xfffeu ? (words[at] >> 16) & 0x7fffu : (words[at] >> 24) & 15u;
        require(length < count - at, "encoded instruction bounds");
        if (opcode == 0xfffeu && length && words[at + 1] == 0x42415443u) ++tables;
        at += length + 1;
    }
    require(tables == 1, "real CTAB metadata generated");
    printf("PASS %s actual bytecode=%zu bytes CTAB=%u\n", profile, code.size, tables);
    vkd3d_shader_free_messages(messages); vkd3d_shader_free_shader_code(&code);
}
int main(void) {
    valid("float4x4 transform; float4 main(float4 position : POSITION) : POSITION { return mul(transform, position); }", "vs_3_0", 0xfffe0300u);
    valid("float4 tint; float4 main(float2 uv : TEXCOORD0) : COLOR0 { return tint * float4(uv, 0.5, 1.0); }", "ps_3_0", 0xffff0300u);
    struct vkd3d_shader_code code = {0}; char *messages = NULL;
    require(compile("float4 main( broken syntax", "ps_3_0", &code, &messages) < 0, "invalid HLSL rejected");
    require(!code.code && !code.size && messages && strstr(messages, "error"), "real diagnostic without success output");
    vkd3d_shader_free_messages(messages); vkd3d_shader_free_shader_code(&code);
    puts("PASS compiler failure diagnostics and output cleanup");
}
