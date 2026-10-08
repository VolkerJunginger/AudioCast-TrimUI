"""Exercise real menu actions, data settings, busy guard and icon restoration."""
import os,shutil,subprocess,tempfile,sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent;build=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='ac-settings-') as d:
    sd=Path(d);app=sd/'Apps/AudioCast';shutil.copytree(root/'Apps/AudioCast',app);(app/'bin').mkdir();(app/'cores').mkdir()
    for n in ['audiocast-cksum','audiocast-core-probe','audiocast-settings','linkaudio-send','audiocast-session','alsa-probe']:shutil.copy(build/n,app/'bin'/n)
    (app/'cores/mgba-link_libretro.so').write_bytes(b'fixture')
    for state in ['on','off']:(app/('icon-'+state+'.png')).write_bytes(state.encode())
    (sd/'RetroArch').mkdir();(sd/'RetroArch/ra64.trimui').write_text('#!/bin/sh\nexit 0\n');(sd/'RetroArch/ra64.trimui').chmod(0o755)
    (sd/'Emus/GB').mkdir(parents=True);(sd/'RetroArch/retroarch.cfg').write_text('')
    launcher=sd/'Emus/GB/launch.sh';original=b'#!/bin/sh\nRA_DIR=/mnt/SDCARD/RetroArch\n$RA_DIR/ra64.trimui -L gambatte_gb_libretro.so "$*"\n';launcher.write_bytes(original)
    def cli(*args,ok=True):
        r=subprocess.run(args,cwd=app,capture_output=True,text=True);assert (r.returncode==0)==ok,r.stdout+r.stderr;return r.stdout.strip()
    menu=str(app/'bin/audiocast-settings');helper=['sh',str(app/'settings.sh')]
    assert cli(*helper,'audio-value')=='1'
    cli(menu,'--render',str(sd/'menu.ppm'));assert (sd/'menu.ppm').read_bytes().startswith(b'P6\n1024 768\n')
    cli(menu,'--change','0');assert (app/'enabled').exists() and (app/'icon.png').read_bytes()==b'on'
    cli(menu,'--change','1');assert cli(*helper,'audio-value')=='0'
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='fms-gba'
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='dmgo-gb' and 'PPQN=16' in (app/'cable/config.txt').read_text()
    cli(menu,'--change','3',ok=False);assert 'PPQN=16' in (app/'cable/config.txt').read_text()
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='gba-clock'
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='stepper-gba' and cli(*helper,'get-ppqn')=='24'
    for ppq in [48,96,4,6,12,24]:
        cli(menu,'--change','3');assert cli(*helper,'get-ppqn')==str(ppq)
        cli(menu,'--render',str(sd/'stepper-menu.ppm'))
    for bad in ['2','8','16','192','$(touch HACKED)']:
        before=(app/'cable/config.txt').read_bytes();cli(*helper,'set-ppqn',bad,ok=False);assert (app/'cable/config.txt').read_bytes()==before
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='fms-clock' and cli(*helper,'get-ppqn')=='2'
    for ppq in [3,4,6,8,1,2]:
        cli(menu,'--change','3');assert cli(*helper,'get-ppqn')==str(ppq)
    cli(*helper,'set-ppqn','24',ok=False)
    cli(menu,'--change','2');assert cli(*helper,'get-clock')=='off'
    cli(menu,'--change','0');assert launcher.read_bytes()==original and (app/'icon.png').read_bytes()==b'off'
    (app/'settings.txt').write_text('LINK_AUDIO=$(touch HACKED)\nBAD=1\n');assert cli(*helper,'audio-value')=='1' and not (app/'HACKED').exists()
    (app/'settings.txt').unlink();(app/'settings.txt').symlink_to(sd/'sentinel');(sd/'sentinel').write_text('unchanged');cli(*helper,'set-audio','off',ok=False);assert (sd/'sentinel').read_text()=='unchanged'
    (app/'settings.txt').unlink()
    busy=Path('/tmp/audiocast-v0.2b');assert not busy.exists();busy.mkdir()
    try:
        cli(menu,'--change','1',ok=False);assert not (app/'settings.txt').exists()
        cli(*helper,'set-clock','stepper-gba',ok=False)

    finally:busy.rmdir()
    assert not list(sd.rglob('*.log')) and not list(app.glob('*.tmp.*'))
print('PASS: menu actions, audio/clock persistence, GB launcher and dynamic icons restored, data not executed, symlink/busy protection, no logs')
