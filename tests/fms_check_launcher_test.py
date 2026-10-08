"""Exercise the stock launcher -> test proxy -> regular AudioCast launch chain."""
import json,os,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='audiocast-fms-check-') as t:
    sd=Path(t)/'SD with spaces';app=sd/'Apps/LINK4BRICKFMSCheck';regular=sd/'Apps/LINK4BRICK'
    shutil.copytree(ROOT/'Apps/LINK4BRICKFMSCheck',app)
    shutil.copytree(ROOT/'Apps/LINK4BRICK',regular)
    for p in app.iterdir():
        if p.suffix=='.sh' or '.audiocast-' in p.name:p.chmod(0o755)
    (regular/'enabled').touch()
    (app/'cores').mkdir();(app/'cores/mgba_libretro.so').write_bytes(b'core stand-in')
    (app/'cores/mgba_libretro.so').chmod(0o755)
    rom=sd/'Roms/GBA/My FMS.gba';rom.parent.mkdir(parents=True);rom.write_bytes(b'ROM stand-in; not executed')
    (app/'rom-path.txt').write_text('Roms/GBA/My FMS.gba\n')
    runtime=Path(t)/'runtime';runtime.mkdir()
    (runtime/'ra.cfg').write_text('config_save_on_exit = "false"\n')
    (runtime/'override.cfg').write_text('log_to_file = "false"\n')
    gba=sd/'Emus/GBA';gba.mkdir(parents=True)
    for n in ['cpufreq.sh','cpuswitch.sh']:(gba/n).write_text('printf "%s\\n" '+n+' >> "$STOCK_TRACE"\n')
    # Reuse the real four-launcher fixture when available; this fallback has the
    # same verified argument form and executes no hardware-specific operations.
    fixture=Path('/Users/volker/Downloads/Archiv/tg3040_Brick_SD_base_package_20241105/Emus/GBA/launch.sh')
    original=fixture.read_text() if fixture.exists() else '#!/bin/sh\nRA_DIR=/mnt/SDCARD/RetroArch\nEMU_DIR=/mnt/SDCARD/Emus/GBA\ncd "$RA_DIR"\n$EMU_DIR/cpufreq.sh\n$EMU_DIR/cpuswitch.sh\n$RA_DIR/ra64.trimui -v -L $RA_DIR/.retroarch/cores/mgba_libretro.so "$*"\n'
    original=original.replace('/mnt/SDCARD',str(sd))
    # The firmware fixture quotes neither its directory paths nor CPU script
    # calls. Quote only those fixture paths to also exercise this test's spaces.
    original=original.replace('RA_DIR='+str(sd)+'/RetroArch','RA_DIR="'+str(sd)+'/RetroArch"')
    original=original.replace('EMU_DIR='+str(sd)+'/Emus/GBA','EMU_DIR="'+str(sd)+'/Emus/GBA"')
    original=original.replace('cd $RA_DIR/','cd "$RA_DIR/"')
    original=original.replace('$EMU_DIR/cpufreq.sh','/bin/sh "$EMU_DIR/cpufreq.sh"').replace('$EMU_DIR/cpuswitch.sh','/bin/sh "$EMU_DIR/cpuswitch.sh"')
    original=original.replace('$RA_DIR/.retroarch/cores/mgba_libretro.so','"$RA_DIR/.retroarch/cores/mgba_libretro.so"')
    # Rewrite only active lines exactly as AudioCast does.
    run='\n'.join(line if line.lstrip().startswith('#') else line.replace('$RA_DIR/ra64.trimui','"${AC_APP}/run-ra.sh"') for line in original.split('\n'))
    (gba/'launch.sh.audiocast-run').write_text(run)
    for p in gba.iterdir():p.chmod(0o755)
    ra=sd/'RetroArch/ra64.trimui';ra.parent.mkdir()
    ra.write_text('#!/usr/bin/env python3\nimport json,os,sys\nfrom pathlib import Path\nPath(os.environ["CAPTURE"]).write_text(json.dumps({"args":sys.argv[1:],"clock":os.environ.get("AUDIOCAST_CLOCK_SOCKET"),"cwd":os.getcwd()}))\n');ra.chmod(0o755)
    # Replace only the audio supervisor in this host fixture. run-ra.sh remains
    # the production file and receives the real stock launcher's arguments.
    (regular/'run.sh').write_text('#!/bin/sh\nAC_APP="$(dirname "$0")"\nAC_SD="$(cd "$AC_APP/../.." && pwd)"\nAC_RUN="$TEST_RUN"\nexport AC_APP AC_SD AC_RUN\nscript="$1"\nshift\nexec /bin/sh "$script" "$@"\n')
    capture=Path(t)/'capture.json';trace=Path(t)/'stock-trace.txt'
    env=dict(os.environ,CAPTURE=str(capture),STOCK_TRACE=str(trace),TEST_RUN=str(runtime),AUDIOCAST_CLOCK_SOCKET='/tmp/must-not-attach')
    protected={p:p.read_bytes() for base in [regular,gba] for p in base.rglob('*') if p.is_file()}
    result=subprocess.run(['sh',str(app/'launch.sh')],env=env,capture_output=True)
    assert result.returncode==0 and result.stdout==result.stderr==b'',result
    data=json.loads(capture.read_text());args=data['args']
    assert data['clock'] is None and Path(data['cwd']).resolve()==ra.parent.resolve()
    assert args[-3:]==['-L',str(app/'cores/mgba_libretro.so'),str(rom)]
    assert args[args.index('--save')+1]==str(app/'saves') and args[args.index('--savestate')+1]==str(app/'saves')
    assert args[args.index('--config')+1]==str(runtime/'ra.cfg')
    assert trace.read_text().splitlines()==['cpufreq.sh','cpuswitch.sh']
    assert all(p.read_bytes()==b for p,b in protected.items())
    assert not list(sd.rglob('*.log'))
    (regular/'enabled').unlink();capture.unlink()
    assert subprocess.run(['sh',str(app/'launch.sh')],env=env).returncode!=0 and not capture.exists()
print('PASS: stock CPU/directory setup, private core/saves, unchanged installed helpers/launchers, inactive clock, disabled-route refusal, no log files')
