# Read-only independent TASK-0013 reviews

Lead owns all implementation, builds/tests/Ledger. User authorized delegation.
Agents envelope_policy and envelope_review inspected source and final changes;
neither edited files, built/tested, modified Ledger or protected checkouts.

Production audit confirmed exact16-byte side-buffer read at decoded offset36,
unchanged signed counts, sentinel-only sign extension and ordinary zero extension;
no block cursor commit. Persistent final destination receives alias slots at
block0 offsets4/12, including zero values. Reset must precede destination/owner
lifetime ending. Root tags are presence values. Full Load_XAssetListCustom loads
strings immediately and is excluded. Side-buffer aliases are outside blocks;
later production fixup resolution is not certified. Private disk metadata remains
stack-referencing until next read overwrites it; no later dereference in this gate.
Moved3 declarations and envelope/helper body bytes match baseline. Narrow Android
selection retains actual source, while full normal Windows expander compiles.
Header assertions follow actual pointer width, independent of legacy flag selector.
Heavy flag-off graphics assertions were retained; light envelope-only test passes.

Adversarial reviewer confirmed uint64 present-array minimum disk/native bounds,
negative count rejection, null-positive presence semantics, exact finite-reader
publication and persistent aliases. Reviewer requested snapshot checks BEFORE
reset for short-read failure and preflight; lead added seeded aliases/nonzero
delay marker, all cursor/shadow/output/read/counter comparisons before reset.
Final reviewer confirmed gap closed and no remaining bounded-gate blocker.
Post-read bad_alloc/engine async/refill/fixup/string/asset paths remain outside
certification. Fixtures include truncation/extreme signed counts, arithmetic
beyond32-bit totals, exact/one-over bounds, native footprint failure, unaligned
prefix, trailing bytes, all pointer-tag classes and short-reader exception.
Next boundary recommended: block4 packed script-string pointer table, before
Load_TempString loop/string bytes/interning, not whole Load_ScriptStringList.
