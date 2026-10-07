#!/usr/bin/env python3
"""Fetch the pinned upstream core, apply the small integration patch and copy driver."""
from pathlib import Path
import argparse, subprocess, shutil
SHA = "3a5e34be33dc7f8f707e5bc9db69e8a430046f21"
p = argparse.ArgumentParser()
p.add_argument("destination", type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
if not a.destination.exists():
    subprocess.run(["git", "init", str(a.destination)], check=True)
    subprocess.run(["git", "-C", str(a.destination), "remote", "add", "origin", "https://github.com/mgba-emu/mgba.git"], check=True)
    subprocess.run(["git", "-C", str(a.destination), "fetch", "--depth=1", "origin", SHA], check=True)
    subprocess.run(["git", "-C", str(a.destination), "checkout", "--detach", "FETCH_HEAD"], check=True)
def git(*args):
    return subprocess.run(["git", "-C", str(a.destination), *args], check=True, capture_output=True, text=True).stdout.strip()
if git("rev-parse", "HEAD") != SHA:
    raise SystemExit("mGBA source does not match the tested upstream commit")
# Only apply missing exact patches; never reset an existing source checkout.
for name in ["mgba.patch", "mgba-input.patch"]:
    patch = root / "sync" / name
    check = subprocess.run(["git", "-C", str(a.destination), "apply", "--reverse", "--check", str(patch)], capture_output=True)
    if check.returncode:
        subprocess.run(["git", "-C", str(a.destination), "apply", "--check", str(patch)], check=True)
        subprocess.run(["git", "-C", str(a.destination), "apply", str(patch)], check=True)
target = a.destination / "src/platform/libretro/audiocast"
target.mkdir(parents=True, exist_ok=True)
for name in ["clock.h", "mgba-clock.h", "mgba-clock.c", "mgba-audio.h", "mgba-audio.c"]:
    shutil.copyfile(root / "sync" / name, target / name)
print("Prepared pinned mGBA with AudioCast clock peripheral")
