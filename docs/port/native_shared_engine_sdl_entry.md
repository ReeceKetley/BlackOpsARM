# Shared native engine runtime and real SDL entry

Status: all three entry/runtime TUs compile independently as actual ARM64 PIC
objects and expose the intended genuine entry symbols. Sources are frozen with
the manifest below. No shared engine library, APK, Activity startup or rendered
run is claimed here.

## Scope and ownership

Only the following source files are owned by this change:

- tools/port/engine/native_engine_main.cpp: thin dedicated console wrapper.
- src/platform/android/native_engine_runtime.h: NativeEngineEntry enum and the
  single RunNativeEngine(NativeEngineEntry,int,char**) interface.
- src/platform/android/native_engine_runtime.cpp: moved actual boot/frame body.
- src/platform/android/native_sdl_engine_entry.cpp: exported C SDL_main wrapper.

All four are explicit CP1252/CRLF, without a BOM. Source hashes and byte sizes
are in build-android-audit/native_registry_closure/sdl-engine-entry1/source-manifest.json.
No protected checkout, asset, policy admission, assertion or decompiled engine
body was changed. There is no independent engine implementation or substitute
Com_Init/Com_Frame/quit endpoint. Root owns CMake/Gradle/Manifest, controller
adoption publication and global ledger/current-state updates.

## Actual entry behavior

DedicatedConsole retains argc2, original chdir/root installation, environment
BO1_ENGINE_USER_ROOT with /data/local/tmp/blackopsarm-user-data default, genuine
STDIN_FILENO console producer and the exact original command-line bytes:

```
+set bo1_headless 1 +set bo1_zombies 1 +set dedicated 1 +set logfile 2 +set net_ip 127.0.0.1
```

SDLActivity requires argc4: SDK program argv0 ("app_process"), gameRoot, userRoot,
--rendered. Pinned SDL2.30.11 nativeRunMain constructs that program argv0;
getMainSharedObject selects the engine library independently. Both roots
must be explicit existing absolute POSIX directories, bounded by PATH_MAX,
resolvable by realpath, and readable/searchable; userRoot must also be writable.
The SDL path never selects the CLI user-root fallback or installs stdin.

Before calling SDL Android getters it requires genuine SDLJavaSetupComplete
and root-owned SDLActivityAdopted publication. It then reads the actual native
thread JNI environment and SDL Activity and checks actual isFinishing and
isDestroyed methods inside a JNI local frame. Invalid argv, roots or adoption/
context/lifecycle return1 before a watchdog is spawned or engine paths are
installed. This matters because a rejected SDL_main can return while the
Android Activity process remains alive. Dedicated valid startup preserves the
original watchdog placement, 1..3600 environment parsing, expiration _Exit2,
and release/acquire actual initialization-return publication.

The SDL command line explicitly selects bo1_headless0/dedicated0 with the same
Zombies/log/network parameters. It calls the existing Sys_InitializeHeadlessPolicy;
that owner's current rendered rejection remains unchanged and terminal. There
is no automatic fallback to headless and no successful rendered admission.

Both branches share the original order: path/command-line/policy installation,
critical sections, main thread, native diagnostics, actual user-root process
guard, PMem, tracking, localization, parse/Dvars/timing/system info, actual
Com_Init, real initialization-return/diagnostic publication, and an infinite
real Platform_InputFrame/Com_Frame/DiagnosticsFrame loop. Requested quit still
invokes canonical Com_Quit_f and aborts if that terminal owner wrongly returns.
The canonical shutdown/process-guard cleanup owners remain unchanged.

For SDL, the existing canonical IN_Init inside Com_Init installs the real
InitializeRenderedInput event consumer/Android console producer, then actual
GPad_InitAll. Installing it early in this entry would cause its existing
running-owner check to reject duplicate installation. The runtime therefore
lets that canonical stage retain ownership. It does not publish input/window/
video/device readiness itself.

## Verification and limits

Evidence directory: build-android-audit/native_registry_closure/sdl-engine-entry1.
All three TUs compiled independently using existing production entry compile
commands with NDK30.0.16248370/API28, GNU++20/O0, signed char, MS extensions,
Win32 compatibility include and UBSan/no recovery. BO1_STARTUP_SERVICES and
BO1_STRING_POOL_GATE were removed; BO1_NATIVE_FULL_DATABASE and
BO1_NATIVE_COM_BOOT remain. -fPIC was added for shared library compatibility. Output was redirected to the evidence directory; no
production target or saved full428 ELF was rebuilt or replaced.

All three compile logs report exit0. The runtime compile retains the existing
cscr_main.h138 const-struct warning; no unrelated source was changed to silence
it. llvm-readelf confirms ELF64 little endian AArch64 relocatable objects. main
and SDL_main each reference the same RunNativeEngine mangled symbol, defined
once in the runtime object. The runtime references real unresolved canonical
Com_Init(char*), Com_Frame(), Com_Quit_f(), policy and actual SDL/JNI service
symbols for the final production link. SDL_main is a GLOBAL DEFAULT C function
export in its object. This is object/symbol proof, not a final shared-library dynamic
symbol or runnable-package claim. Compile and symbol logs are retained.

Error-path checks are source/interface audit, not executed Android tests. SDK
root/context rejection, no-SDK-stdin ownership, dedicated exact command-line
bytes and unchanged common engine order were inspected. No stubbed boot test,
fixture success or fake JNI adoption is substituted for real execution.

## Wiring and remaining gates

Root should compile native_engine_runtime.cpp once in a shared PIC static
runtime target used by CLI and the SDK engine shared library. Link
native_sdl_engine_entry.cpp and sdl_activity_bootstrap.cpp directly into the
shared library so SDK dlsym/JNI exports are retained. Link the same genuine
engine archives and pinned SDL2.30.11/DXVK-Native1.9.2 providers; no Vulkan upgrade.
The Java SDK getArguments contract supplies exactly gameRoot/userRoot/
--rendered after SDL's actual program argv0 ("app_process"). There is no library
identity normalization. Root supplies the read-only actual Activity
adoption getter and application-UID accessible/imported storage.

Positive actual SDK adoption, rendered policy/provider closure, shared-library
link and exported dynamic symbols, APK packaging/launch, real window/input/
audio/GPU/client frames and rendered Zombies play remain unverified. The proven
full428 dedicated map run is prior parent evidence; this object check does not
extend that run's scope to rendered play.

## Frozen source manifest

- tools/port/engine/native_engine_main.cpp: SHA256 6D9C6A3F527F1D8EFB8F8526259E58D951775C27A3D530A36EF6998C6A3B5985, 274 bytes, CP1252 CRLF.
- src/platform/android/native_engine_runtime.h: SHA256 79E90457DC6E33D05EE9485B6471FF3C7A35EDBE979C5E70F4EABBD73D8BFA0C, 409 bytes, CP1252 CRLF.
- src/platform/android/native_engine_runtime.cpp: SHA256 6040D82EDD93924ACA938DD516BA5F778C2BE14AD6F5E4476A10E3F6B4111DCA, 7677 bytes, CP1252 CRLF.
- src/platform/android/native_sdl_engine_entry.cpp: SHA256 51978F7356DCFAE2A1264EBE1E68A583C52B262B4B0171FE01D9B45C1B20DDE7, 392 bytes, CP1252 CRLF.

The runtime check uses the root-owned genuine adoption query declaration:
controller_service.h SHA256
CCBAF7910E453D37F2200BBD0E7E47251EB1CC5439300736E5886B86AC738AD6;
controller_service.cpp SHA256
3A3C957E381CCAB8ABB0F7D06D2299337E49629D725B557E8E97B9F34C77D85D.
Its acquire getter reads the separate adoption publication; only successfully
validated actual AdoptSDLActivityJava performs its release publication. Manual
SetupSDLControllerJava does not publish Activity adoption. These two dependency
files are root-owned and were read-only to this entry change.

Final compile commands with exit codes are retained in compile-commands.json;
per-TU compile/symbol logs and combined entry-symbols.log are beside it. All four
entry source hashes were rechecked after compilation and match the frozen
manifest. Ledger status and doctor were read-only checked; doctor reports healthy.
Root retains ownership of ledger fact/current-state publication.


A later read-only SDK audit confirmed nativeRunMain738 in the pinned
SDL/src/core/android/SDL_android.c prepends app_process and separately dlopens
the selected shared object. Only the header/report argv0 wording was corrected;
no argv normalization or readiness behavior was added. Root subsequently
guarded SDK-only includes/validation with BO1_NATIVE_FULL_DATABASE to preserve
the historical non-full profile. The native runtime source was read-only to this
correction; its current root-owned source hash is reflected above.
