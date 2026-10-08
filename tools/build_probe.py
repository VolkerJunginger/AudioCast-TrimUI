#!/usr/bin/env python3
"""Build optional private FMS probe against a configured STATIC mGBA build."""
from pathlib import Path
import argparse, shlex, subprocess, sys
p = argparse.ArgumentParser()
p.add_argument("build", type=Path)
p.add_argument("--output", type=Path, default=Path("fms-probe"))
p.add_argument("--stepper", action="store_true", help="Build synthetic STEPPER SI interrupt test")
p.add_argument("--gb", action="store_true", help="Build generated Game Boy serial regression test")
p.add_argument("--serial", action="store_true", help="Build synthetic bar/serial transport regression test")
p.add_argument("--audio", action="store_true", help="Build fixed-rate audio regression test")
p.add_argument("--synthetic", action="store_true", help="Build public synthetic core test instead of private FMS probe")
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
flags = (a.build / "CMakeFiles/mgba.dir/flags.make").read_text()
args = []
for name in ["C_DEFINES", "C_INCLUDES"]:
    args += shlex.split(next(x.split(" = ", 1)[1] for x in flags.splitlines() if x.startswith(name)))
libs = ["-framework", "Foundation"] if sys.platform == "darwin" else ["-ldl", "-lpthread"]
subprocess.run(["cc", "-std=c11", "-D_POSIX_C_SOURCE=200809L"] + args +
    [str(root / ("tests/stepper_clock_test.c" if a.stepper else "tests/gb_clock_test.c" if a.gb else "tests/serial_clock_test.c" if a.serial else "tests/audio_rate_test.c" if a.audio else "tests/core_clock_test.c" if a.synthetic else "tests/fms_probe.c")), str(root / ("sync/mgba-audio.c" if a.audio else "sync/mgba-clock.c")),
     str(a.build / "libmgba.a"), "-lm"] + libs + ["-o", str(a.output)], check=True)
