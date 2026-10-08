"""Generate antialiased Inter glyph atlases; font is licensed under SIL OFL."""
from pathlib import Path
import urllib.request,hashlib
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parent.parent
font_path=root/'assets/Inter.ttf'
font_sha='29160a80ff49ddcab2c97711247e08b1fab27a484a329ce8b813d820dc559031'
if not font_path.exists():
    font_path.parent.mkdir(exist_ok=True)
    url='https://raw.githubusercontent.com/google/fonts/0b58fb370093f9a9f4ff785d94405710b79de67c/ofl/inter/Inter%5Bopsz,wght%5D.ttf'
    font_path.write_bytes(urllib.request.urlopen(url,timeout=30).read())
if hashlib.sha256(font_path.read_bytes()).hexdigest()!=font_sha:raise SystemExit('Pinned Inter font verification failed')
out=root/'Apps/LINK4BRICK/ui';out.mkdir(exist_ok=True)
entries=[];pixels=bytearray()
for size in [20,24,30,40]:
 font=ImageFont.truetype(str(root/'assets/Inter.ttf'),size)
 for code in range(32,127):
  c=chr(code);x,y,r,b=font.getbbox(c);w,h=max(1,r-x),max(1,b-y)
  image=Image.new('L',(w,h));ImageDraw.Draw(image).text((-x,-y),c,font=font,fill=255)
  entries.append((len(pixels),w,h,x,y,round(font.getlength(c))))
  pixels.extend(image.tobytes())
(out/'font.bin').write_bytes(pixels)
header=f'// Generated from Inter under SIL OFL 1.1. See LICENSES/Inter-OFL.txt.\nstatic constexpr unsigned kFontBytes={len(pixels)};\nstruct Glyph {{ unsigned offset; int w,h,x,y,advance; }};\nstatic constexpr Glyph glyphs[][95]={{\n'
for i in range(4):header+='{' + ','.join('{'+','.join(map(str,e))+'}' for e in entries[i*95:(i+1)*95])+'},\n'
header+='};\n';(root/'src/ui_font.h').write_text(header)
logo=Image.open(root/'Apps/LINK4BRICK/icon-on.png').convert('RGBA').resize((80,80),Image.Resampling.LANCZOS)
(out/'logo.rgba').write_bytes(logo.tobytes())
print('Generated',len(pixels),'font pixels and 80px display logo; original icons unchanged')
