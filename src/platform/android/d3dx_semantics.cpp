#include <platform/win32_compat/native_d3dx9_api.h>
#include <cstring>

namespace {
constexpr HRESULT invalidData = static_cast<HRESULT>(0x88760b59u); // June2010 D3DXERR_INVALIDDATA
unsigned registerType(DWORD token) {
    return ((token & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT)
        | ((token & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);
}
HRESULT semantics(const DWORD *shader, _D3DXSEMANTIC *result, UINT *count, bool output) {
    if (!shader) return D3DERR_INVALIDCALL;
    const DWORD stage = shader[0] & 0xffff0000u;
    if (stage != 0xfffe0000u && stage != 0xffff0000u) return invalidData;
    // The original material loader requests vs_3_0/ps_3_0. Older encodings
    // require their own declaration/implicit-register policies, not empty data.
    if ((shader[0] & 0xffffu) != 0x0300u) return E_NOTIMPL;
    const bool pixel = stage == 0xffff0000u;
    _D3DXSEMANTIC found[MAXD3DDECLLENGTH]{};
    UINT used = 0;
    unsigned colors = 0;
    bool depth = false;
    constexpr unsigned maximumWords = 1u << 20;
    unsigned at = 1;
    while (at < maximumWords && shader[at] != D3DSIO_END) {
        const DWORD instruction = shader[at];
        const unsigned opcode = instruction & D3DSI_OPCODE_MASK;
        unsigned length = (instruction & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
        if (opcode == D3DSIO_COMMENT)
            length = (instruction & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
        if (length >= maximumWords - at) return invalidData;
        if (opcode == D3DSIO_DCL) {
            if (length != 2) return invalidData;
            const unsigned type = registerType(shader[at + 2]);
            if ((!output && type == D3DSPR_INPUT) || (output && !pixel && type == D3DSPR_OUTPUT)) {
                if (used == MAXD3DDECLLENGTH) return invalidData;
                found[used++] = {(shader[at + 1] & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT,
                    (shader[at + 1] & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT};
            }
        } else if (output && pixel && opcode != D3DSIO_COMMENT && opcode != D3DSIO_DEF
                   && opcode != D3DSIO_DEFI && opcode != D3DSIO_DEFB) {
            for (unsigned parameter = 1; parameter <= length; ++parameter) {
                const DWORD token = shader[at + parameter];
                if (!(token & 0x80000000u)) continue;
                const unsigned type = registerType(token), index = token & D3DSP_REGNUM_MASK;
                if (type == D3DSPR_COLOROUT) {
                    if (index >= 4) return invalidData;
                    colors |= 1u << index;
                } else if (type == D3DSPR_DEPTHOUT) depth = true;
            }
        }
        at += length + 1;
    }
    if (at == maximumWords) return invalidData;
    if (pixel && output) {
        for (unsigned color = 0; color < 4; ++color)
            if (colors & (1u << color)) found[used++] = {D3DDECLUSAGE_COLOR, color};
        if (depth) found[used++] = {D3DDECLUSAGE_DEPTH, 0};
    }
    if (result) std::memcpy(result, found, used * sizeof(*result));
    if (count) *count = used;
    return S_OK;
}
}
extern "C" HRESULT WINAPI D3DXGetShaderInputSemantics(const DWORD *shader, _D3DXSEMANTIC *result, UINT *count) {
    return semantics(shader, result, count, false);
}
extern "C" HRESULT WINAPI D3DXGetShaderOutputSemantics(const DWORD *shader, _D3DXSEMANTIC *result, UINT *count) {
    return semantics(shader, result, count, true);
}
