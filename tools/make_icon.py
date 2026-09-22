"""Render the original AudioCast geometric icon (Pillow, build time only).

Transparent 256px StockUI icontop: handheld, audio waveform and linked loops.
This is an AudioCast design, not an official Ableton logo.
StockUI format: https://github.com/trimui/assets_brick/blob/main/README.md
"""
from pathlib import Path
import sys
from PIL import Image, ImageDraw

state = sys.argv[2] if len(sys.argv) > 2 else 'off'
if state not in ('on', 'off'):
    raise SystemExit('state must be on or off')
S = 4
image = Image.new('RGBA', (256*S, 256*S))
d = ImageDraw.Draw(image)
white = '#F4F7F9'
cyan = '#65E0D4' if state == 'on' else '#8C969E'
dark = '#172833'

def box(coords, radius, fill, outline=None, width=1):
    d.rounded_rectangle(tuple(int(v*S) for v in coords), radius*S,
                        fill=fill, outline=outline, width=width*S)

def line(coords, color, width):
    d.line([(int(x*S), int(y*S)) for x,y in coords], fill=color, width=width*S,
           joint='curve')

# Portrait handheld silhouette and screen.
box((20,31,121,222),20,dark,white,7)
box((34,49,107,120),8,cyan)
line([(41,87),(52,87),(59,70),(69,101),(79,76),(86,87),(100,87)],dark,5)
box((38,154,77,166),3,white)
box((51,141,63,179),3,white)
d.ellipse((87*S,148*S,101*S,162*S),fill=cyan)
d.ellipse((97*S,172*S,111*S,186*S),fill=cyan)
box((53,201,87,205),2,white)
# Two interlocking loops make the connection legible even at small sizes.
link = Image.new('RGBA',image.size)
ld = ImageDraw.Draw(link)
for coords in [(142,97,204,130),(176,97,238,130)]:
    ld.rounded_rectangle(tuple(v*S for v in coords),16*S,outline=cyan,width=7*S)
ld.line([(187*S,111*S),(193*S,116*S)], fill=white, width=7*S)
link = link.rotate(32, resample=Image.Resampling.BICUBIC, center=(190*S,114*S))
image.alpha_composite(link)
d = ImageDraw.Draw(image)
# Small audio bars under the connection.
for x,h in [(149,14),(165,28),(181,42),(197,28),(213,14)]:
    box((x,182-h/2,x+7,182+h/2),3,white)
# Filled turquoise status dot for ON; hollow gray dot for OFF.
d.ellipse((207*S, 32*S, 239*S, 64*S), fill=dark)
d.ellipse((213*S, 38*S, 233*S, 58*S),
          fill=cyan if state == 'on' else dark, outline=cyan, width=3*S)
image = image.resize((256,256),Image.Resampling.LANCZOS)
output = Path(sys.argv[1])
output.parent.mkdir(parents=True,exist_ok=True)
image.save(output, format='PNG', optimize=True)
print('Created transparent 256x256 AudioCast icon:',output)
