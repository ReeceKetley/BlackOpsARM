# Native reverb O0/O2 equivalence audit

2026-10-14. Bounded fixture for the source-scoped optimization of canonical `src/sound/snd_radverb.cpp`. Production source and algorithms are unchanged by this audit. Root owns build configuration, installation, actual APK performance measurement, and Ledger/STATE.

## Build evidence

Evidence: `build-android-audit/native_reverb_optimization1/`.

`actual_compile_entry.json` captures the existing native database command. `build_fixture.py` parses that Windows command without changing target, headers, defines, prefix, language or safety flags. The canonical source is compiled twice with terminal `-O0` / `-O2` and macro-renamed external reverb entrypoints so both versions coexist in one executable. The canonical `snd_dsp.cpp` supplies `I_fclamp`; no independent engine implementation is copied. Each command and stdout/stderr is preserved. O0, O2, DSP, fixture and executable link all exit 0.

Both versions retain NDK30 Clang, Android28 ARM64, `_DEBUG=1`, `-g`, `-fPIC`, `-finstrument-functions`, `-fsanitize=undefined`, and `-fno-sanitize-recover=all`. No fast-math, reassociation, float-contract override or assertion suppression is introduced. Undefined-symbol and disassembly files prove retained `Assert_MyHandler`, instrumentation entry/exit, stack protector and UBSan abort references in both objects. O2 can prove away some checks; remaining checks still abort. O2 folds the paired trigonometric calls to `sincosf`, making execution comparison useful.

Canonical source SHA256: `snd_radverb.cpp` 95e08e55a8b4a1c399abafa7bd71f3d35ff21b371d9f4c643a3f8c025c4a3eab; `snd_radverb.h` 5641144f555a9af71014412f9e5c6daf2fecc4f32fd128a9aa62f09e13cef8dd. Full source/fixture/object/executable hashes are in `source_hashes.json` and `fixture_hashes.json`.

## Numerical fixture

`fixture.cpp` uses real `snd_rv_params`, real aligned `snd_rv_state`, canonical default parameters, and actual `SND_RvFrame` entrypoints. It zeroes storage/padding consistently, initializes legal parameters and persistent state, and checks outputs, parameters and every byte of the full persistent state after each block. Any assertion prints its location and aborts. Instrumentation hooks are empty fixture sinks, with instrumentation calls retained in the compiled engine objects. They do not represent the production hook cost.

Eight scenarios each execute 200 blocks per version with counts 1/64/128/256/512. Cases cover silence, impulse followed by silence, deterministic nonzero input, early/late sizes .1/1/16, both delay matrices, smoothing0/1, diffusion0/1, valid8k/44.1k/48k/96k rates, changing reflection/filter/size/smoothing/matrix parameters, and two in-place channel cases matching the alias behavior of `SDXA2RadverbEffect::Process`. Initial delayIndex32760 forces wrap in the first few blocks; 38440 frames per case extend through another ring traversal. The exact total is reported by the executable. Outputs must also remain finite.

Acceptance is bitwise equality (tolerance0), including all persistent delay samples, filter states, coefficients/delays, parameters, delay bases and index. A first differing byte/sample causes immediate failure; no tolerance is silently relaxed.

## Execution status and limits

Root ran the exact ARM64 executable on the authorized HONOR tablet; `device-run.txt` records all eight scenarios PASS and exit0. Across1600 calls per version,307520 frames and1230080 output floats, including767836 nonzero outputs, outputs/parameters/full persistent state were bitwise equal (tolerance0). Real ARM64 fixture execution passed without assertion or UBSan failure. This is numerical fixture evidence, separate from actual APK runtime proof. The fixture is bounded valid-input evidence; it does not exhaust all asset parameters, assert complete engine correctness, measure audio CPU reduction, or prove30FPS/playable Zombies. Root must report actual APK gains separately.

## Preserved first failure

The initial Python `shlex(posix=False)` attempt incorrectly split adjacent `-I"path with spaces"` arguments. Compilation failed before an object was produced. `first-failure-command-parser.log/json` retain that failure. The script now uses Windows `CommandLineToArgvW`; all final compile/link commands pass.
