# Original model/font/ammo/compass integration and native raw font bounds

2026-10-13. Full-engine-link185 compiles selected sources and fails remaining
genuine dependencies. Actual Com_Init/Com_Frame/rendering unexecuted.

Selected complete cg_ammocounter, cg_compassfriendlies_mp, xmodel_load_obj and
r_font_load_obj; removed cg_compass's remaining query-only restriction, enabling
its canonical full body. Original weapon-index, compass HUD/registration and
model/font loader owners now retained, without dummy success or fabricated assets.

Native raw font loader previously allocated payload+24 then placed payload after
the wider native Font_s. It now allocates sizeof(Font_s)+payload, checking signed
allocation-size overflow. The disk header remains16 bytes, glyph record24 bytes,
with original four-byte offsets/counts. Native reads require exact byte counts;
glyph count/height/string offsets/ranges and bounded terminators are validated
before native pointer construction/material registration. Invalid input returns
null and closes the file. Hunk allocations remain hunk-owned, including rejected
metadata; this does not add independent freeing or claim recovery of that hunk.
Windows loader branch and file format unchanged.

Evidence:

-184 compiles all five original sources;185 compiles font allocation/read/bounds
  adaptation and reaches genuine linker dependencies.
- takeover_model_font_compass_windows1 passes five whole normal TUs on x86/x64,
  commands/manifests/build-result.json retained.
- raw-font-build2/run1 on HONOR UBSan runs the actual production R_LoadFont body
  with explicit synthetic file, allocation and material-registration fixtures.
  Valid native header/payload allocation, glyph data/pointers/names, close/two
  registrations, short reads, invalid count/offset/unterminated names and all
  allocation canaries pass. Fixtures allocate heap blocks with boundary canaries;
  this is parser evidence, not real filesystem/Hunk/material/GPU initialization.
  No exhaustive font corpus, OOM/huge-size runtime or renderer result claimed.

Only new parser test binary transferred; no assets scanned/copied. Native model,
compass/ammo HUD and actual raw font/material execution remain unverified. Further
decompiled model disk/runtime pointer and stride assumptions require audit before
execution; compilation is not blanket asset loader certification. Continue real
audio/cinematic/demo/client/game/physics/UI/renderer/network/recovery links shown
by185, then validate authentic production startup and pending native lifecycles.
