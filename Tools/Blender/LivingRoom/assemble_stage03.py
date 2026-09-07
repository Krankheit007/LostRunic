"""Unretouched source/render comparisons for the Level 3 gate."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw,ImageFont
OUT=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Stage03')
font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',24)
def sheet(items,name):
    canvas=Image.new('RGB',(1440,((len(items)+1)//2)*620),'#eeece7')
    draw=ImageDraw.Draw(canvas)
    for i,(path,title) in enumerate(items):
        x,y=(i%2)*720,(i//2)*620
        draw.text((x+15,y+12),title,font=font,fill='#303030')
        im=ImageOps.contain(Image.open(path).convert('RGB'),(700,560))
        canvas.paste(im,(x+(720-im.width)//2,y+52+(560-im.height)//2))
    canvas.save(OUT/name)
sheet([(Path('D:/GameDesign/DontForgetAdele/livingroom.png'),'原始概念图'),
       (OUT/'01_Concept.png','三级细节 · Blender 概念距离预览')],'Review_Comparison.png')
sheet([(OUT/f,t) for f,t in [
    ('Fireplace_Front.png','壁炉 · 柱饰与大线脚'),
    ('ArchBookcase_Front_Open.png','柜门 · 边框与把手随铰链运动'),
    ('WindowConsole_Front_Open.png','长桌 · 抽屉把手与桌沿'),
    ('CoffeeTable_Front.png','茶几 · 桌沿分层')]],'Review_Details.png')
print('Stage03 review sheets saved')
