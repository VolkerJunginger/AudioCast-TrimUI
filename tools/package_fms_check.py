#!/usr/bin/env python3
"""Package the additive core check using the already-verified ARM64 core."""
import argparse,json,hashlib,shutil,struct,subprocess,sys,tempfile,zipfile
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser()
parser.add_argument('--core',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
header=args.core.read_bytes()[:64]
assert header[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',header,18)[0]==183
with tempfile.TemporaryDirectory() as t:
    stage=Path(t);app=stage/'Apps/AudioCastFMSCheck'
    shutil.copytree(ROOT/'Apps/AudioCastFMSCheck',app)
    (app/'cores').mkdir();shutil.copyfile(args.core,app/'cores/mgba_libretro.so')
    subprocess.run([sys.executable,str(ROOT/'tools/make_icon.py'),str(app/'icon.png'),'on'],check=True)
    # Original geometric T badge distinguishes the temporary test app.
    icon=Image.open(app/'icon.png').convert('RGBA');draw=ImageDraw.Draw(icon)
    draw.ellipse((194,16,248,70),fill='#ECA760',outline='#172833',width=3)
    draw.line([(207,30),(235,30)],fill='#172833',width=5)
    draw.line([(221,30),(221,57)],fill='#172833',width=5)
    icon.save(app/'icon.png',optimize=True)
    for p in app.iterdir():
        if p.is_file() and (p.suffix=='.sh' or '.audiocast-' in p.name):
            p.chmod(0o755);subprocess.run(['sh','-n',str(p)],check=True)
    (app/'cores/mgba_libretro.so').chmod(0o755)
    shutil.copyfile(ROOT/'docs/FMS_LAUNCH_TEST.md',stage/'README.txt')
    shutil.copyfile(ROOT/'THIRD_PARTY.md',stage/'THIRD_PARTY.txt')
    (stage/'LICENSES').mkdir()
    for p in [ROOT/'LICENSE',ROOT/'LICENSES/mGBA-MPL-2.0.txt',ROOT/'LICENSES/mGBA-inih.txt']:
        shutil.copyfile(p,stage/'LICENSES'/p.name)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(stage.rglob('*')):
            if p.is_file():z.write(p,p.relative_to(stage))
    with zipfile.ZipFile(args.output) as z:
        assert z.testzip() is None
        assert all(n.startswith(('Apps/AudioCastFMSCheck/','LICENSES/')) or n in ['README.txt','THIRD_PARTY.txt'] for n in z.namelist())
        assert not any(n.lower().endswith(('.gba','.gb','.sav','.srm','.log','.pak')) for n in z.namelist())
        assert json.loads(z.read('Apps/AudioCastFMSCheck/config.json'))['label']=='FMS Launch Test'
        assert not any('/bin/' in n for n in z.namelist()), 'No duplicate audio components'
    args.output.with_suffix(args.output.suffix+'.sha256').write_text(hashlib.sha256(args.output.read_bytes()).hexdigest()+'  '+args.output.name+'\n')
print('PASS: ARM64 core, ZIP integrity, additive StockUI layout, no replacement AudioCast/launchers/ROMs/saves/logs/.pak')
