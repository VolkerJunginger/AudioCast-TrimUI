"""Offline tests for the additive FMS profile; no SD card or ROM is executed."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix="ac-sync-test-") as tmp:
    sd = Path(tmp) / "SD with spaces"
    app = sd / "Apps/AudioCastSync"
    shutil.copytree(ROOT / "Apps/AudioCastSync", app)
    rom = sd / "Roms/GBA/My FMS.gba"
    rom.parent.mkdir(parents=True)
    rom.write_bytes(b"private ROM stand-in; never executed")
    (app / "rom-path.txt").write_text("Roms/GBA/My FMS.gba\n")
    (app / "cores").mkdir()
    core = app / "cores/mgba_libretro.so"
    core.write_text("core stand-in"); core.chmod(0o755)
    capture = Path(tmp) / "launch.json"
    run = app / "run.sh"
    run.write_text("#!/usr/bin/env python3\nimport json, os, sys\nfrom pathlib import Path\nPath(os.environ['CAPTURE']).write_text(json.dumps({'args':sys.argv[1:],'socket':os.environ['AUDIOCAST_CLOCK_SOCKET'],'ppqn':os.environ['AUDIOCAST_PPQN'],'offset':os.environ['AUDIOCAST_OFFSET_US']}))\n")
    # launch.sh invokes /bin/sh run.sh. Use a tiny shell adapter to the recorder.
    recorder = app / "record.py"
    recorder.write_text(run.read_text())
    run.write_text('#!/bin/sh\nexec python3 "$(dirname "$0")/record.py" "$@"\n')
    env = dict(os.environ, CAPTURE=str(capture))
    def launch(ok):
        capture.unlink(missing_ok=True)
        result = subprocess.run(["sh", str(app / "launch.sh")], env=env, capture_output=True)
        assert (result.returncode == 0) == ok
        assert result.stdout == result.stderr == b""
        assert capture.exists() == ok
    launch(True)
    data = json.loads(capture.read_text())
    assert data["args"] == [str(app / "fms.audiocast-run"), str(rom)]
    assert data["ppqn"] == "2" and data["offset"] == "0"
    assert (app / "saves").is_dir()
    assert not (sd / "Emus").exists()
    (app / "clock-settings.txt").write_text("PPQN=4\nOFFSET_US=-12000\n")
    launch(True)
    assert json.loads(capture.read_text())["offset"] == "-12000"
    (app / "clock-settings.txt").write_text("PPQN=$(touch unexpected)\n")
    launch(False); assert not (Path.cwd() / "unexpected").exists()
    (app / "clock-settings.txt").write_text("PPQN=24\n")
    launch(False)
    (app / "clock-settings.txt").write_text("PPQN=2\nOFFSET_US=hello\n")
    launch(False)
    (app / "clock-settings.txt").write_text("PPQN=2\nOFFSET_US=0\n")
    for bad in ["--", "1-2", "250001", "-250001", "999999999999999999999999999999999"]:
        (app / "clock-settings.txt").write_text("PPQN=2\nOFFSET_US=" + bad + "\n")
        launch(False)
    (app / "clock-settings.txt").write_text("PPQN=2\nOFFSET_US=0\n")
    core.unlink();launch(False)
    # Verify dedicated core/save arguments using a fake RetroArch, not the ROM.
    ra = sd / "RetroArch/ra64.trimui"
    ra.parent.mkdir()
    ra.write_text(recorder.read_text().replace("'socket':os.environ['AUDIOCAST_CLOCK_SOCKET'],'ppqn':os.environ['AUDIOCAST_PPQN'],'offset':os.environ['AUDIOCAST_OFFSET_US']", "'unused':0"));ra.chmod(0o755)
    runtime = Path(tmp) / "runtime";runtime.mkdir()
    (runtime / "ra.cfg").write_text("unchanged main copy\n")
    (runtime / "override.cfg").write_text("log_to_file = \"false\"\n")
    env.update(AC_RUN=str(runtime), AC_APP=str(app), AC_SD=str(sd))
    subprocess.run(["sh", str(app / "fms.audiocast-run"), str(rom)], env=env, check=True)
    args = json.loads(capture.read_text())["args"]
    assert args[-3:] == ["-L", str(core), str(rom)]
    assert args[args.index("--save")+1] == str(app / "saves")
    assert 'config_save_on_exit' not in (runtime / "ra.cfg").read_text()
    assert not list(sd.rglob("*.log"))
print("PASS: FMS paths with spaces, separate launcher/core/saves, numeric config, invalid input rejection, no launcher edits/logs")
