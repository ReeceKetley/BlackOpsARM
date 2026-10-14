# Canonical resident WMAv2 decoding and voice lifetime

2026-10-13. This batch supersedes the initial native sound-driver report's WMA unsupported status. Driver and decoder sources are frozen. Production integration/Windows comparisons remain root-owned; complete SD_Init/Com_Init and Android physical audio are not proved by these tests.

## Actual asset contract and read-only census

This T5 source has no SndAssetBankHeader type. Its actual disk contract is the56-byte snd_asset within60-byte LoadedSound. Load_snd_asset reads the cumulative seek-table words followed by separately aligned/borrowed audio data. Original iSND_CreateVoice supports nonstream WMAv2 tag0x161, mono2230/stereo4096 fixed packets, 16-bit decoded cumulative-byte dpds values, and PacketCount*packetSize compressed bytes. Other header block_size/buffer_size fields concern stream windows and do not describe WMAv2 packet framing.

The read-only batch census covered all29103 IWD wav headers:29102 MSADPCM and1 PCM16. Strict bounded inline LoadedSound candidate scans of all86 zone/Common FastFiles found41080 WMAv2 and2504 MSADPCM records, including duplicated references across zones. This is a candidate-record census, not a full canonical loader traversal or a unique-asset count. WMA had exactly two metadata classes:29933 mono44100Hz/2230-byte packets and11147 stereo48000Hz/4096-byte packets. Every accepted WMA candidate had monotonic, whole-frame dpds values ending at frame_count*channels*2. No census errors. tools/port/tests/retail_sound_type_census.py reproduces the batch; retail-sound-type-census.json stores metadata/counts under build-android-audit/native_registry_closure. No original asset was changed or retransmitted.

## Genuine decoder and necessary EOS repair

Unchanged official FFmpeg7.1.2 source and licenses are under third_party/ffmpeg-7.1.2. Official archive https://ffmpeg.org/releases/ffmpeg-7.1.2.tar.xz has SHA256089bc60fb59d6aecc5d994ff530fd0dcb3ee39aa55867849a2bbc4e555f9c304; UPSTREAM.json records provenance. The actual build disables GPL, nonfree, version3, external codec libraries, demuxers, network, programs, filters, hardware assembly and unused decoders. Only WMAV2_DECODER is enabled in generated config_components.h. libavcodec61.19.101 and libavutil are actual native static libraries. The full retained release contains unused GPL source files, but CONFIG_GPL/CONFIG_NONFREE/CONFIG_VERSION3 are all0 in the selected library build. LGPL2.1+ licenses and full source remain available; distribution must retain the corresponding source, notices, local patch and relinking materials for these static libraries.

The provider uses actual avcodec send/receive APIs, copied/padded packet storage, real xWMA codec flags31 from official libavformat/xwma.c, canonical bitrates and real decoded planar float frames converted into owned interleaved float samples. It checks packet lengths, packet-size class, cumulative table monotonicity/alignment, final decoded byte count, all frame/channel/rate/format bounds, finite samples, pinned library version and allocation errors. Failure leaves the result empty. It never accepts upstream's unrelated silence fallback or fills a short decode with zeros.

A real codec defect was exposed by the mono class: upstream generic WMA EOS emits transform overlap but discards the remaining compressed last_superframe. Automatic generic4096-sample initial-delay removal then yielded45056 frames against canonical/Microsoft45568, with a real missing tail. Disabling delay removal and trimming from sample0 produced shifted audio, so that apparent count-only pass was rejected. The actual Microsoft SDK xWMAEncode decoder established correct45568/49152 frame counts and waveform timing without an audio device.

The narrow reviewed EOS patch decodes the actually buffered final compressed frame using the original wma_decode_frame routine, then emits the actual transform overlap. Corrupt buffered data still fails. Generic actual initial-delay handling remains enabled. The canonical dpds endpoint removes only actual decoded transform padding; excess greater than4096 frames or a short decode fails. No success padding is created. Upstream vendor bytes remain unchanged: tools/port/prepare_native_wmav2.py makes a build-only source copy and checks/applies tools/port/patches/ffmpeg-7.1.2-wmav2-eos.patch, SHA25606d05d5278b144504d139ef202301236d4d2164d43e0f4639a08e4c51d27b173. This deliberate change is documented rather than represented as stock upstream behavior.

Primary codec references: https://github.com/FFmpeg/FFmpeg/blob/n7.1.2/libavformat/xwma.c and https://github.com/FFmpeg/FFmpeg/blob/n7.1.2/libavcodec/wmadec.c .

## Canonical production integration

Native iSND_CreateVoice accepts canonical resident WMA, decodes it before publishing an SDK voice, and selects actual IEEE_FLOAT WAVEFORMATEX with its original channel/rate/frame count. A separate pervoice owned decoded buffer survives SDK borrowing. The original source DSP, master/world/Radverb routing, original infinite loop count, EOS flag, start delay, pause, pitch and sample-count consumers are retained. SD_StopVoice destroys/quiesces the actual voice before clearing its decoded owner. Submit failure destroys that voice before freeing its owner; create/decode failure publishes no owner. Compressed asset memory/seek metadata remains borrowed during synchronous decoding. Windows uses its original WMA/XAudio2 branch. New source/header code is guarded; original driver was saved as CP1252.

The native bridge still rejects direct raw WMA factory calls, since FAudio's SDL WMA fallback is unusable. Genuine decoding occurs at the canonical asset owner and submits a correctly framed owned float buffer to the already validated real mixer. PCM16/MSADPCM paths and existing streaming windows remain unchanged. Original WMA is nonstream; adding streamed WMA is not claimed. No other unsupported original codecs are silently accepted.

Root recipe: add_subdirectory(android/cmake/native-wmav2 <binary-dir>); select src/platform/android/wma_decoder.cpp with actual original driver; target_link_libraries(bo1_native_entry_state PUBLIC bo1_native_wmav2_codec) and final executable PRIVATE bo1_native_wmav2_codec. Its imported codec/util targets depend on a real custom producer, export generated include headers, and link m/atomic/android/mediandk dependencies. Existing actual FAudio/SDL/source selection remains in place. The wrapper is explicitly limited to the audited Windows/MSYS2, NDK30.0.16248370/API28/ARM64 host contract and rejects other toolchains. D:/bo1-native-codec-audit is a verified no-spaces junction to this exact ARM worktree for upstream make; preparation rejects an unrelated existing target. No protected directory is a producer. A future portable/toolchain-specific recipe needs corresponding actual adaptation, rather than assuming these artifacts match another ABI.

Standalone producer:
```
D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe -S android/cmake/native-wmav2 -B build-android-audit/native_wmav2_cmake -G Ninja -DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe -DCMAKE_TOOLCHAIN_FILE=D:/Android/Sdk/ndk/30.0.16248370/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=28 -DCMAKE_BUILD_TYPE=Debug
D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe --build build-android-audit/native_wmav2_cmake --target bo1_native_wmav2_build
```
wmav2-cmake-configure2.log and wmav2-cmake-build1.log pass with actual static outputs. The exact configure/compiler/options are durable in tools/port/build_native_wmav2.sh. Root full-production rebuild was released only after this producer reached terminal exit0.

## Focused evidence and limits

The driver actual ARM64 TU compiles in snd_driver-wma-native-compile1.log. The decoder fixture links actual engine modified-zlib inflate sources, the existing bounded FastFile owner and actual native avcodec/util. HONOR AKWJ9X5116G04618 reads already staged code_post_gfx.ff, validates the two packet-class payload SHA256s and decodes under UBSan. wma-decoder-honor-eos-run1.log and final frozen-decoder wma-decoder-honor-final.log pass exact49152 stereo and45568 mono frames, finite/nonzero audio, and rejects corrupted packets, bad tables and truncated compressed storage. No audio device or Java owner was initialized by this codec fixture; it proves actual software decode, not physical playback.

The actual Microsoft SDK oracle decoded the same two canonical packet-class payloads. All185512-frame windows from HONOR output were compared with Microsoft PCM16 means/RMS, including the final mono512-frame tail. wma-microsoft-oracle-final.log preserves the final comparison. Maximum mean error9.67481584e-7 and RMS error3.25462493e-6 are within the3e-5 PCM16-rounding tolerance. This compares window statistics throughout both waveforms; it is not a claim of bit-identical float samples or exhaustive decode of every record.

Reproduction from the ARM worktree:
```
./tools/port/build_native_wma_fixture.ps1
python tools/port/tests/prepare_wma_microsoft_oracle.py
adb -s AKWJ9X5116G04618 push build-android-audit/native_registry_closure/native-wma-decoder-tests /data/local/tmp/blackopsarm-native-engine-entry47-1/native-wma-decoder-tests
adb -s AKWJ9X5116G04618 shell 'chmod 755 /data/local/tmp/blackopsarm-native-engine-entry47-1/native-wma-decoder-tests && cd /data/local/tmp/blackopsarm-native-engine-entry47-1 && ./native-wma-decoder-tests /data/local/tmp/blackopsarm-game-data/zone/common/code_post_gfx.ff'
python tools/port/tests/compare_wma_microsoft_oracle.py <captured-honor-log>
```
The focused host actual FAudio graph fixture additionally passes twice in faudio-graph-host-run5.log, including its original PCM/MSADPCM proof and an owned interleaved float infinite-loop voice. It frees that float buffer only after actual safe destruction and verifies subsequent callback counters stay fixed. This isolates the real SDK float format/borrowing boundary; it does not execute iSND_CreateVoice itself. Actual production WMA creation, concurrent sound-worker lifecycle, all resident clip lengths, full retail loading, SD_Init/Com_Init and physical Android output remain unexecuted.

Preserved failed evidence includes upstream mono short-decode runs1/2, the rejected manual-delay count-only experiment run3, original EOS comparison diagnostics, and initial MSYS configure failures (missing PATH utilities, GCC host assembler issues). The selected actual LLVM MinGW host compiler avoids that assembler issue. wmav2-cmake-configure1.log records the first wrapper check incorrectly querying legacy NDK CMAKE variables; it was corrected to actual ANDROID_NDK/ANDROID_PLATFORM_LEVEL. No engine assertion or native Java readiness requirement was weakened.
