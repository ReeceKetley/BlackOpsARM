# Native wavelet byte-stream reads

The actual SDL APK full432 renderer reaches real wavelet texture decoding and
fails UBSan at `Wavelet_DecompressLevel`'s initial unsigned-short load from
`0x006e28200003`. Matching shared library BuildId is
`18b490afe10c4adfcf1496edb689c4218c25b85a`; the retained launch/crash logs are
`sdk-native-entry432-launch1.log` and `sdk-native-launch432-crash1.txt` under
`build-android-audit/native_registry_closure`.

This alignment is part of the original codec. `Image_LoadWavelet` starts with the
smallest mip and carries one compressed-data cursor through later mips. The raw
RGB base mip consumes three channel bytes before the next mip initializes its
16-bit bitstream. `Wavelet_ConsumeBits` subsequently advances by arbitrary byte
counts. The actual crash dump shows the RGB bytes `33 3D 41`, followed by the
initial bitstream bytes `82 9A`; the valid original little-endian word is `9A82`.
An alignment requirement on that word would reject a legitimate byte stream.

The canonical source contains exactly two typed unsigned-short reads from this
byte stream, identical to BlackOps64. Android now assembles exact little-endian
bytes for the initial two-byte word and the existing four-byte refill lookahead.
The refill casts each byte to unsigned before shifting, including the high byte
shift by 24. Both original Windows expressions remain in their original branches.
Only `src/gfx_d3d/r_image_wavelet.cpp` changed. Huffman tables, coefficient
arithmetic, channel order, mip reconstruction, state widths, cursor advancement,
assertions and retail asset bytes remain unchanged. The read extent remains the
original 2/4 bytes; this does not introduce a different truncated-stream contract.

Frozen source SHA256:
`BF83E0540958BE0EC4C02B98D726F283DFACCFBF8FB5C655024BAFA3948FDC99`.
Before/after bytes and manifest are preserved under
`build-android-audit/native_registry_closure/wavelet-byte-read1`.
Actual production-command native whole-TU compilation passes with NDK30/API28,
PIC/debug/function instrumentation and UBSan/no recovery retained. Normal Windows
x86/x64 comparisons also pass with the established `/permissive /DWIN32=1`
configuration.

The bounded fixture calls the actual final `Wavelet_DecompressLevel` owner for
30 cases: all permitted channel/bpp combinations, even/odd payload starts, three
compressed-byte patterns including high bytes and the observed RGB prefix, and
in-place 1→2→4 mip reconstruction. It records every final pixel hash, compressed
cursor displacement, remaining bit index and value. Output tail canaries check
the original pixel extent. The actual normal Windows x64 object produces the
golden 30 rows and exits 0. The same actual instrumented Android object is linked
into `wavelet-tests`, with clearly fixture-only assertion/instrumentation
providers; it is not a copied decoder or GPU/provider substitute.

The single HONOR fixture run exits 0 with `PASS actual wavelet decoder cases=30`.
All 30 pixel-hash/cursor/bit/value rows exactly match the normal Windows golden
rows, and every even/odd pair is identical. `honor-fixture1.log` and
`compare-result.json` retain the actual result. Root owns the next real APK launch;
these decoder checks alone do not establish texture upload, rendering or returned
application initialization. Source ownership is frozen at the recorded hash.
