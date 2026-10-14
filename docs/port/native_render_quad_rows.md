# Native render quad vertex rows

SDK453 stage25 actually reached CA_ACTIVE at77766ms, then UBSan reported r_rendercmds.cpp1926 index2 outside const float[2]. The matching follow1 crash is actual pid21708/SDLThread21781, SIGABRT; BuildId3f2190fa0bf9c426fed9ced747deea16d1bb2510. Its stack passes CG_CompassDrawRound, CG_DrawRotatedPic, CG_DrawRotatedPicPhysicalW and R_AddCmdDrawQuadPicW. The immutable SDK453 binary predates this repair.

R_AddCmdDrawQuadPicW now reads verts[cornerIndex][0/1] on native non-Windows builds. The bounded same-class audit before freeze found four identical first-row-crossing reads in immediate producer CG_DrawRotatedQuadPic; those now read verts[i][0/1]. Adjacent immediate producer DevGui_DrawQuad also flattened its const int[2] input across all eight coordinates; native reads now use vtxs[0..3][0/1], preserving every int-to-float conversion. Root expanded ownership to batch both proven producers before freeze. Both canonical four-corner loops, math/order, signatures, color/material/w payload and command stride are unchanged. Windows retains the original expressions. Typed offsets equal 2*corner+component for all eight coordinates.

Native Android and Windows x86/x64 whole translation units pass for all three changed files. Native preprocessed target bodies are exact before bodies after only the fourteen documented coordinate substitutions. Windows x86/x64 bodies match both before sources and read-only BlackOps64 oracle exactly. Branch reversal recreates each entire before source byte-for-byte, preserving all prior native R_CMD_BYTES, command alignment/span guards, color handling and other source behavior. CP1252 high bytes and existing mixed endings are preserved.

The bounded audit covered active quad/matrix reads in r_rendercmds.cpp and immediate quad producers in cg_drawtools.cpp, cg_draw_indicators.cpp, devgui_util.cpp and ui_main.cpp, plus the immediate consumer RB_DrawQuadPicCmd and GfxCmdDrawQuadPic declaration. Actual-path CG_DrawRotatedPicPhysicalW constructs float verts[4][2] with typed rows; backend consumes cmd->verts[0..3][0/1] and advances the stored command span. Other callers construct typed 4x2 arrays. ConvertQuatToMat in cg_drawtools has valid first-row0..2 and typed rows1/2; no repair needed. r_rendercmds debug shader constant row is indexed before its float-pointer conversion. No additional active first-row quad/matrix crossing was found in this bounded scope. Unrelated backend vertex helpers and previously disabled bodies were not audited broadly. DevGui runtime is conditional and is not claimed tested; upstream DevGui_DrawBevelBox wide scalar stores remain outside this bounds-repair scope.

Frozen hashes:

- src/gfx_d3d/r_rendercmds.cpp: 3E915D95285BCB4F82D55ACFA073652D7012C5585EA2FC95179C253AB8BB311B
- src/cgame/cg_drawtools.cpp: B066A1B4E4881214BFB5922C64DDF3E2A987BDEA098A04F0B1513CAA2D91EFE3
- src/devgui/devgui_util.cpp: 9EBBCF2A43BDE26D120C80265B4166B7C3C56F7226BDA1CE89B3C7E0E4BB76D7

Evidence: build-android-audit/native_render_quad_rows1/source-manifest.json, audit-results.json, render/check-results.json, producer/check-results.json and devgui/check-results.json retain source snapshots, diffs, actual compiler commands/logs and target bodies. Original failure: build-android-audit/native_registry_closure/sdk-native-entry453-follow1-only.log and sdk-native-launch453-follow1-crash.txt.

Root owns SDK454 stage26 integration and actual device observation. Patched execution is pending; this source/compile proof does not establish complete3D or playable Zombies. The overall goal remains incomplete.
