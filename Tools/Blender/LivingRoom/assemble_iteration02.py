from pathlib import Path
from PIL import Image,ImageOps,ImageDraw,ImageFont
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
OUT=BASE/'Iteration02/Previews'
font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',24)
items=[('D:/GameDesign/DontForgetAdele/livingroom.png','原始参考图'),
       (BASE/'SurfaceRevision/Previews/01_Concept.png','迭代前'),
       (OUT/'01_Concept.png','迭代后 · 同相机、同光照')]
canvas=Image.new('RGB',(2160,640),'#eeece7');draw=ImageDraw.Draw(canvas)
for i,(path,title) in enumerate(items):
    draw.text((i*720+15,12),title,font=font,fill='#303030')
    im=ImageOps.contain(Image.open(path).convert('RGB'),(700,575))
    canvas.paste(im,(i*720+(720-im.width)//2,52+(575-im.height)//2))
canvas.save(OUT/'Reference_Before_After.png')
canvas=Image.new('RGB',(1440,1240),'#eeece7');draw=ImageDraw.Draw(canvas)
for i,(path,title) in enumerate([(BASE/'SurfaceRevision/Previews/Sofa_Front.png','沙发 · 迭代前'),(OUT/'Sofa_Front.png','沙发 · 减薄软包、修正靠垫及织物 UV'),(BASE/'SurfaceRevision/Previews/01_Concept.png','整体 · 迭代前'),(OUT/'01_Concept.png','整体 · 暗木、墙面、地板与褪色地毯')]):
    x,y=i%2*720,i//2*620;draw.text((x+12,y+10),title,font=font,fill='#303030')
    im=ImageOps.contain(Image.open(path).convert('RGB'),(700,560));canvas.paste(im,(x+(720-im.width)//2,y+50+(560-im.height)//2))
canvas.save(OUT/'Iteration_Detail_Review.png')
