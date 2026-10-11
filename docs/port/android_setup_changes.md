# Android setup file inventory — 2026-10-11

All project edits are in `BlackOpsARM/`. No commit or push was made. The exact
machine-readable inventory, including ignored generated build files and the
inherited source checkout, is in [setup-verification.json](../../.ledger/setup-verification.json).

## Existing files changed

| File | Change |
| --- | --- |
| `.gitignore` | Ignore Android outputs/local settings and Ledger's local database/archive; retain wrapper JAR and durable memory. |
| `.ledger/AGENT-INSTRUCTIONS.md` | Add Android/parent ownership and current setup scope. |
| `.ledger/GOAL.md` | Fill initialized goal with native T5 ARM64 requirements and constraints. |
| `.ledger/DECISIONS.md` | Record source baseline, devices, renderer, platform policy and Ledger ownership. |
| `.ledger/STATE.md` | Record setup, verified build, limitations and first compilation task. |
| `.ledger/ledger.jsonl` | Append decisions, blocker, next task and verification using Ledger CLI. |
| `.ledger/ledger.db` | Ledger CLI updates the existing local SQLite index; ignored. |

The pre-existing `.ledger/.lock`, `archive/` and `entities/` are retained.

## New setup files

```text
AGENTS.md
android/settings.gradle
android/build.gradle
android/gradle.properties
android/gradlew
android/gradlew.bat
android/gradle/wrapper/gradle-wrapper.jar
android/gradle/wrapper/gradle-wrapper.properties
android/app/build.gradle
android/app/src/main/AndroidManifest.xml
android/app/src/main/java/com/blackopsarm/dev/DevelopmentActivity.java
src/platform/android/CMakeLists.txt
src/platform/android/environment.cpp
src/platform/android/README.md
src/platform/win32_compat/README.md
docs/port/android_arm64.md
docs/port/android_setup_changes.md
.ledger/setup-verification.json
```

New local, ignored files: `android/local.properties` (SDK path) and
`.ledger/setup-baseline.json` (before-state hashes). The new `.git` file links
this directory to BlackOps64's worktree registration. Git also created the
`android-arm64` branch and its independent administrative worktree/index; `main`
and the Windows index are unchanged. These are Git metadata, not engine files.

## Materialized and generated files

Git materialized the existing committed engine/source/docs/data tree at
`76edfd5` in the new worktree. This is the normal worktree checkout sharing
repository history, not a separate engine fork or a second source tree inside
the Android project. All inherited tracked content matches the base, except
the Android-only `.gitignore` additions above.

The build generated ignored files under `android/.gradle/`,
`android/app/.cxx/` and `android/app/build/`, including the debug APK, native
library, lint reports and the extracted inspection library in
`android/app/build/verification/`. Exact file paths are listed in the verification
JSON. No original game assets or renderer binaries were imported.

Outside the project, Android SDK Manager installed `cmake;3.22.1` under
`D:/Android/Sdk/cmake/3.22.1` and its SDK package metadata. Gradle updated its
normal user caches and local development-signing material as needed. No
Windows or DXVK source, branch or index was edited, and no root Ledger file
was changed.
