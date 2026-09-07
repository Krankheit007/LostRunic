from pathlib import Path
import json
from PIL import Image,ImageOps,ImageDraw,ImageFont
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Delivery')
OUT=BASE/'Previews'
font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',23)
canvas=Image.new('RGB',(1440,620),'#eeece7');draw=ImageDraw.Draw(canvas)
for i,(p,title) in enumerate([(Path('D:/GameDesign/DontForgetAdele/livingroom.png'),'原始概念图'),(OUT/'01_Concept.png','材质分区与贴图候选 · 不模拟 UE NPR')]):
    draw.text((i*720+14,12),title,font=font,fill='#303030')
    im=ImageOps.contain(Image.open(p).convert('RGB'),(700,560))
    canvas.paste(im,(i*720+(720-im.width)//2,52+(560-im.height)//2))
canvas.save(OUT/'Review_Comparison.png')
im=Image.open(OUT/'01_Concept.png');im.resize((im.width//4,im.height//4),Image.Resampling.LANCZOS).save(OUT/'Thumbnail_25pct.png')
mask=Image.open(BASE/'Textures/T_LR_FloorMass_SMK.png')
mask.getchannel('G').save(OUT/'SMK_G_Floor_Debug.png')
manifest=json.loads((BASE/'Manifest.json').read_text())
assert len(manifest['assets'])==81 and all(not x['errors'] for x in manifest['assets'])
print('81 verified FBXs; source comparison and SMK.G debug saved')
