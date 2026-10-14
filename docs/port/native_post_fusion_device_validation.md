# Actual tablet bloom/color fusion validation

SDK477 first paired comparison rejected: 2,304,000 pixels, 10,892 different,
508 alpha mismatches, maximum RGB difference4. Original rendering continued.
SDK478/stage49 adds an independent reconstruction of the original two passes.
That control matches every original RGBA channel exactly: 9,216,000 channels,
zero differing pixels, maximum RGB/alpha difference0. The fused output still
fails:10,381 differing pixels,508 alpha mismatches, maxRGB4/maxAlpha4.
The comparison setup is therefore validated for this captured frame; shader
fusion is NOT accepted and mode2 was never used. No FPS improvement claimed.

BuildId6b360adc344127529cbeb9326e1ef118ea3e35a8, PID17350, marker48,
genuine CA_ACTIVE71910ms and touch-operated room. Seven providers packaged and
assets reused. Native control source SHA25624C0C27BDF1F8EF6E69EDEBA28C6FF6FEE1033CE83777F703DEBBF673360A741.
Mode0 restored and queried in the same live process; console/IME closed and
fresh screenshot captured. GPU readback temporarily stalls metrics; this is
validation overhead, not a fused-path performance measurement.

Evidence: native_registry_closure/sdk-native-entry478-control-validate-follow1-only.log,
control-original-query1-only.log and sdk478-screen-control-console-close2.png.
Original/control/fused RGBA PAMs pulled byte-preserving into
docs/port/evidence/native_post_fusion_device478. Retail assets/shaders untouched.
Next investigate the removed intermediate texture/filter boundary using these
captures. Do not relax image thresholds. Independent small CPU candidate is
source-scoped-O2 rb_shade.cpp with assertions, instrumentation, UBSan and strict
math retained; this has not yet been implemented or measured.
