"""Run the real device engine with timed console input and retained evidence.

Requires an already staged executable, libraries and assets. No files are pushed.
The device timeout bounds the run; its expiry is a failure, never engine success.
"""
import argparse
import json
import shlex
import subprocess
import time
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", required=True)
    parser.add_argument("--serial", required=True)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--game-root", required=True)
    parser.add_argument("--log", required=True, type=Path)
    parser.add_argument("--limit", type=int, default=180)
    parser.add_argument("--trace-postinit", choices=("0", "1"), default="0",
                        help="Keep startup trace scans after genuine init only for a controlled baseline")
    parser.add_argument("--command", action="append", default=[],
                        help="Seconds after launch:actual engine console command")
    args = parser.parse_args()
    if not 1 <= args.limit <= 3600:
        parser.error("Run limit must be 1..3600 seconds")
    if "/" in args.binary or "\\" in args.binary or args.binary in ("", ".", ".."):
        parser.error("Binary must name an already staged executable in the directory")
    schedule = []
    for value in args.command:
        seconds, separator, command = value.partition(":")
        try:
            delay = float(seconds)
        except ValueError:
            parser.error("Command timing must be a number of seconds")
        if not separator or not 0 <= delay < args.limit or not command or any(
                c in command for c in "\r\n\0"):
            parser.error("Each command requires a time within the run and one console line")
        schedule.append((delay, command))
    schedule.sort(key=lambda entry: entry[0])
    result_path = args.log.with_suffix(args.log.suffix + ".json")
    if args.log.exists() or result_path.exists():
        parser.error("Use fresh evidence paths; previous logs are preserved")
    remote = "cd " + shlex.quote(args.directory) + " && " + shlex.join([
        "timeout", str(args.limit), "env", "LD_LIBRARY_PATH=.:/data/local/tmp",
        "BO1_ENGINE_WATCHDOG_SECONDS=60", "UBSAN_OPTIONS=print_stacktrace=1",
        "BO1_TRACE_POSTINIT=" + args.trace_postinit,
        "./" + args.binary, args.game_root,
    ])
    argv = [args.adb, "-s", args.serial, "shell", "-T", remote]
    sent = []
    started = time.monotonic()
    with args.log.open("xb") as output:
        process = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=output,
                                   stderr=subprocess.STDOUT)
        try:
            index = 0
            while process.poll() is None:
                elapsed = time.monotonic() - started
                while index < len(schedule) and elapsed >= schedule[index][0]:
                    delay, command = schedule[index]
                    try:
                        process.stdin.write((command + "\n").encode("utf-8"))
                        process.stdin.flush()
                    except (BrokenPipeError, OSError):
                        # The real engine/ADB endpoint may already be terminal.
                        index = len(schedule)
                        break
                    sent.append({"scheduled_seconds": delay,
                                 "sent_seconds": elapsed, "command": command})
                    index += 1
                time.sleep(0.1)
            exit_code = process.wait()
        finally:
            try:
                process.stdin.close()
            except OSError:
                pass  # Preserve the terminal ADB result when its input pipe closed.
    result = {"argv": argv, "exit_code": exit_code,
              "elapsed_seconds": time.monotonic() - started,
              "sent_commands": sent, "transferred_assets": False}
    with result_path.open("x", encoding="utf-8") as manifest:
        json.dump(result, manifest, indent=2)
        manifest.write("\n")
    print(json.dumps(result, indent=2), flush=True)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
