# Android application-owned asset import and SDL launch

2026-10-14. TASK-0030 Java implementation; no APK/device import or rendered-engine execution is claimed.

## Files and launch contract

`AssetImportActivity` is the intended launcher. It uses Android's genuine
`ACTION_OPEN_DOCUMENT_TREE` read grant and persists that grant before starting
worker I/O. The original folder remains user-owned and is never modified.
Managed copies use `getExternalFilesDir("game")`; native writable settings use
`getFilesDir()/user`. Uninstall removes the app's managed copy/settings, explained
on screen. No retail data is embedded in the APK.

After a committed import, the launcher sends `EngineActivity.EXTRA_GAME_ROOT`
(`com.blackopsarm.dev.GAME_ROOT`) and `EXTRA_USER_ROOT`
(`com.blackopsarm.dev.USER_ROOT`). EngineActivity accepts only the current
canonical app-owned roots, obtains an exclusive private file lock and rechecks
the import receipt/assets before native adoption. `getArguments()` returns
`{gameRoot, userRoot, "--rendered"}`. Actual SDL 2.30.11 prepends the main-library
argument, yielding SDL_main argc=4. Root owns selecting this launcher and
`EngineActivity android:process=":engine"` in the manifest because canonical
engine quit currently exits its process. This report does not claim packaging
or native/rendered launch passes.

The real SDK owns setupJNI, Activity/surface/context/controller initialization.
Existing nativeAdoptSDLActivity occurs once after actual SDK onCreate. Auditing
the matching SDK found API>=24 resumes native work through onStart; the protected
resumeNativeThread hook is now guarded by successful adoption. onResume always
calls its superclass to satisfy Android lifecycle. No second SDK setup is used.
The storage lock remains held until SDK onDestroy completes; process exit also
releases it. Import attempts while the engine owns it fail explicitly.

## Import integrity and failure behavior

The importer copies IWD files under main, BIK files under main/video, FastFiles
under zone and root localization.txt. Retained `game-data-staging/assets.sha256`
contains exactly 266 assets: 57 IWD, 158 FF, 50 BIK and localization.txt. The
production selector includes every one, with English capitalization preserved.
Generated logs, Windows binaries and the local generated playlists_sp.info are
excluded. No fixed known hashes are used to claim the user's assets authentic.

The 73 required original paths consist of the 57 original English IWD volumes,
actual MP/SP startup code+patch zones with their English zones, common_zombie,
Kino and both actual map/common patch files. All exist in the retained staged
manifest and read-only retail reference. This is a startup/Kino storage gate,
not an assertion that all maps, engine boot or rendering will work.

Source document IDs and case-folded paths must be unique. Traversal, separators,
control characters, oversized names, virtual documents and recursive folder
trees are rejected. Canonical target containment rejects symbolic-link escape;
primary local source/destination overlap and identical source/destination inodes
are rejected. Previous managed assets absent from a newly selected source cause
an explicit failure rather than silently mixing asset trees or deleting files.

Before any asset changes, an AtomicFile private receipt is durably replaced with
complete=false and reopened to confirm the saved record. Every changed file is
streamed byte-for-byte into a unique same-directory temporary file, with actual
stream SHA256, exact source/target byte counts, free-space checks (64 MiB reserve)
and file-descriptor sync. Only then does POSIX rename install it. A copied file
is not reread in full: standard file writes, sync and exact byte count establish
the completed copy, while the receipt digest describes the actual input stream.
For an existing potential match, current source and destination SHA256 are both
read and compared before skipping the copy; timestamps alone never trigger a
skip. A completed file is recorded after rename. Cooperative cancellation is
checked during scanning, between stream chunks and immediately before rename;
failed/cancelled imports remain incomplete and cannot launch. Partial temporary
files are removed on failure or a later copy retry. Process death also leaves
the receipt incomplete, and another explicit import can reuse identical files.

Only the full successful import publishes complete=true. Each launch checks
receipt version/root, every recorded file's size/mtime, all required readable
nonempty paths, and that no selected managed assets were omitted from the
receipt. Launch does not hash gigabytes again. Metadata checks detect ordinary
removal/replacement; they are not cryptographic proof against external changes
that deliberately retain both length and timestamp. Explicit reimport performs
the real content comparisons. App-private receipt and lock coordinate both
launcher and engine processes.

## Verification and remaining gates

- Actual JDK21 javac, Java17 source/target, SDK35 android.jar, and read-only
  DXVK-Native/third_party/SDL's actual SDL2.30.11 Java sources compile all four
  owned Java files, exit0. Diagnostics are the usual source/module-path warning
  and SDK deprecated-API notes. Retained `asset_import_java/javac-final.log`.
- A pure-Java check runs production AssetPaths with the retained 266-file manifest
  and read-only retail directory: PASS436. It checks concrete traversal/name
  rejection, selects all 266 originals, confirms all 73 launch requirements are
  original nonempty files, and excludes generated/nonasset paths. It does not
  emulate Android or claim SAF/transaction execution. Retained
  `asset_import_java/AssetPathsChecks.java` and `path-checks.log`.
- Source SHA256/size freeze is `asset_import_java/source-manifest.json`.

No APK install, tablet asset transfer, asset move, file hash sweep on the tablet,
or test of actual SAF providers, UI rotation/cancellation, full/denied storage,
process death, engine relaunch, input/audio/GPU/rendered play was performed here.
Those remain real integration gates. Existing 11.2 GiB staged tablet assets and
all protected reference sources are untouched. Manifest/Gradle/CMake/native code
are outside this Java agent's ownership; root owns their integration and ledger.
