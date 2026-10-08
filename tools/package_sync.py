#!/usr/bin/env python3
"""Create and validate the additive StockUI-only experimental ZIP."""
from pathlib import Path
import argparse, hashlib, json, shutil, struct, subprocess, sys, tempfile, zipfile
p = argparse.ArgumentParser()
p.add_argument("--build", type=Path, required=True)
p.add_argument("--core", type=Path, required=True)
p.add_argument("--link", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--host", action="store_true", help="Local package layout check; not installable on Brick")
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
names = ["linkaudio-send", "audiocast-session", "alsa-probe", "audiocast-cksum"]
def check_arm64(file):
    data = file.read_bytes()[:64]
    assert data[:5] == b"\x7fELF\x02" and data[5] == 1, f"Not ELF64 LE: {file}"
    assert struct.unpack_from("<H", data, 18)[0] == 183, f"Not ARM64: {file}"
if not a.host:
    for file in [a.core] + [a.build / n for n in names]: check_arm64(file)
with tempfile.TemporaryDirectory() as tmp:
    stage = Path(tmp)
    app = stage / "Apps/LINK4BRICKSync"
    shutil.copytree(root / "Apps/LINK4BRICKSync", app)
    for name in ["run.sh", "start-game.sh", "run-ra.sh"]:
        shutil.copyfile(root / "Apps/LINK4BRICK" / name, app / name)
    # The private socket can survive a forced emulator kill; this session owns it.
    run = app / "run.sh"
    runtime_script = run.read_text().replace('/tmp/audiocast-v0.2b', '/tmp/audiocast-fms-sync').replace('/tmp/audiocast.fifo', '/tmp/audiocast-fms-sync.fifo')
    run.write_text(runtime_script.replace('"$AC_RUN/override.cfg"\n  rmdir', '"$AC_RUN/override.cfg" "$AC_RUN/clock.sock"\n  rmdir'))
    (app / "enabled").touch()
    (app / "bin").mkdir(); (app / "cores").mkdir(); (app / "saves").mkdir()
    for n in names: shutil.copyfile(a.build / n, app / "bin" / n)
    shutil.copyfile(a.core, app / "cores/mgba_libretro.so")
    subprocess.run([sys.executable, str(root / "tools/make_icon.py"), str(app / "icon.png"), "on"], check=True)
    for f in list(app.glob("*.sh")) + list(app.glob("*.audiocast-*")) + list((app / "bin").iterdir()) + list((app / "cores").iterdir()):
        f.chmod(0o755)
    for f in list(app.glob("*.sh")) + list(app.glob("*.audiocast-*")):
        subprocess.run(["sh", "-n", str(f)], check=True)
    shutil.copyfile(root / "docs/FMS_LINK_SYNC.md", stage / "README.txt")
    shutil.copyfile(root / "THIRD_PARTY.md", stage / "THIRD_PARTY.txt")
    (stage / "LICENSES").mkdir()
    for source in [root / "LICENSE", root / "LICENSES/mGBA-MPL-2.0.txt", root / "LICENSES/mGBA-inih.txt", root / "LICENSES/Ableton-Link.md"]:
        shutil.copyfile(source, stage / "LICENSES" / source.name)
    shutil.copyfile(a.link / "modules/asio-standalone/asio/LICENSE_1_0.txt", stage / "LICENSES/Asio.txt")
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(a.output, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(stage.rglob("*")):
            if f.is_file(): z.write(f, f.relative_to(stage))
    with zipfile.ZipFile(a.output) as z:
        assert z.testzip() is None
        assert all(n.startswith(("Apps/LINK4BRICKSync/", "LICENSES/")) or n in ("README.txt", "THIRD_PARTY.txt") for n in z.namelist())
        assert not any(n.endswith((".gba", ".gb", ".sav", ".srm", ".log", ".pak")) for n in z.namelist())
        assert json.loads(z.read("Apps/LINK4BRICKSync/config.json"))["label"] == "FMS Link Sync"
        for n in names:
            assert z.getinfo("Apps/LINK4BRICKSync/bin/" + n).external_attr >> 16 & 0o111
    digest = hashlib.sha256(a.output.read_bytes()).hexdigest()
    a.output.with_suffix(a.output.suffix+".sha256").write_text(f"{digest}  {a.output.name}\n")
print("PASS: ZIP integrity, ARM64 (unless --host), StockUI layout, executables, licenses, no ROMs/saves/logs/.pak")
