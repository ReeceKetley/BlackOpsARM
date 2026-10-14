#!/system/bin/sh
# Explicit user-requested local move. No PC transfer, no source deletion command.
set -eu
source_root=/data/local/tmp/blackopsarm-game-data
target_root=/storage/emulated/0/boARM
test "$(readlink -f /sdcard)" = /storage/emulated/0
test -d "$source_root/main"
test -d "$source_root/zone"
test ! -e "$target_root"
test -z "$(pidof full-engine429 || true)"
test -z "$(pidof com.blackopsarm.dev:engine || true)"
printf 'Moving existing assets locally: %s -> %s\n' "$source_root" "$target_root"
mv "$source_root" "$target_root"
test ! -e "$source_root"
test -f "$target_root/localization.txt"
test -d "$target_root/main/video"
test -d "$target_root/zone"
printf 'Shared source ready: %s\n' "$target_root"
