"""Five source crops next to raw Blender renders; no retouching."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw,ImageFont

OUT=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Stage02')
reference=Image.open('D:/GameDesign/DontForgetAdele/livingroom.png').convert('RGB')
font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',24)
small=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',18)
items=[
    ('壁炉台面',(365,170,625,337),'Detail_Fireplace.png'),
    ('茶几陈设',(610,475,850,660),'Detail_CoffeeTable.png'),
    ('单人椅旁杯碟',(506,693,595,795),'Detail_SideTableCup.png'),
    ('右侧圆桌花束',(1230,410,1340,535),'Detail_SideTablePlant.png'),
    ('脚凳上的书',(778,199,862,256),'Detail_Ottoman.png'),
]
w,h=650,460
canvas=Image.new('RGB',(w*2,h*len(items)+70),'#eeece7')
draw=ImageDraw.Draw(canvas)
for row,(title,bounds,filename) in enumerate(items):
    for col,(pic,label) in enumerate([(reference.crop(bounds),title+' · 原图局部'),
            (Image.open(OUT/filename).convert('RGB'),title+' · 二级结构补齐')]):
        x,y=col*w,row*h
        draw.text((x+15,y+10),label,font=font,fill='#292c30')
        pic=ImageOps.contain(pic,(w-22,h-58))
        canvas.paste(pic,(x+(w-pic.width)//2,y+48+(h-58-pic.height)//2))
draw.text((15,h*len(items)+22),'原图裁剪仅用于定位；右侧为实际模型渲染。独立陈设已补主体结构，表面花纹和细装饰尚未制作。',font=small,fill='#42454b')
canvas.save(OUT/'Review_DecorationCompletion.png')
# Compact sheet for the conversation; detailed source comparison remains separate.
thumb=Image.new('RGB',(1320,1170),'#eeece7')
d=ImageDraw.Draw(thumb)
for i,(filename,title) in enumerate([
    ('Detail_Fireplace.png','壁炉台面：烛台、花瓶与花束'),
    ('Detail_CoffeeTable.png','茶几：花瓶、浅碗与小器物'),
    ('Detail_SideTableCup.png','圆桌：杯、杯柄与碟'),
    ('Detail_SideTablePlant.png','右侧圆桌：花瓶与花束')]):
    x,y=(i%2)*660,(i//2)*585
    d.text((x+14,y+12),title,font=font,fill='#292c30')
    pic=ImageOps.contain(Image.open(OUT/filename).convert('RGB'),(642,525))
    thumb.paste(pic,(x+(660-pic.width)//2,y+48+(525-pic.height)//2))
thumb.save(OUT/'Review_AddedProps.png')
print('Decoration completion comparison and compact preview saved.')
