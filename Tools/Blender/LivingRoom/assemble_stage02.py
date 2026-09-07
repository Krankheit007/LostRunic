"""Arrange unretouched renders and the original concept on review sheets."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw,ImageFont
OUT=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Stage02')
FONT=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',25)
SMALL=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',18)

def sheet(items,name,footer=''):
    w,h=720,650;rows=(len(items)+1)//2
    canvas=Image.new('RGB',(2*w,rows*h+62),'#eeece7')
    draw=ImageDraw.Draw(canvas)
    for k,(path,title) in enumerate(items):
        x,y=k%2*w,k//2*h
        draw.text((x+14,y+10),title,font=FONT,fill='#2e3034')
        picture=ImageOps.contain(Image.open(path).convert('RGB'),(w-18,h-55))
        canvas.paste(picture,(x+(w-picture.width)//2,y+48+(h-55-picture.height)//2))
    draw.text((15,rows*h+17),footer,font=SMALL,fill='#3d4044')
    canvas.save(OUT/name)

sheet([(Path('D:/GameDesign/DontForgetAdele/livingroom.png'),'原始场景概念图'),
    (OUT/'01_Concept.png','二级结构 · 采用用户修正后的布局')],
    'Review_Comparison.png','颜色为分区预览；墙体与地板保留用户版本。三级细节、UV、正式贴图和最终 FBX 尚未制作。')
sheet([(OUT/p,t) for p,t in [
    ('Sofa_Front.png','沙发 · 坐垫、靠垫、卷扶手'),
    ('Sofa_Back.png','沙发 · 背部与披毯垂搭'),
    ('ArmchairOchre_Front.png','赭黄扶手椅 · 软包结构'),
    ('ArmchairRusset_Front.png','红褐扶手椅 · 软包结构')]],
    'Review_Seating.png','独立家具近景，用于二级结构验收；不代表最终材质效果。')
sheet([(OUT/p,t) for p,t in [
    ('ArchBookcase_Front.png','拱顶书柜 · 关闭状态'),
    ('ArchBookcase_Front_Open.png','拱顶书柜 · 铰链试开'),
    ('WindowConsole_Front_Open.png','窗下长桌 · 三个抽屉试拉'),
    ('SmallCabinet_Front_Open.png','小柜 · 抽屉与内部结构')]],
    'Review_Casework.png','试开用于检查结构与轴心；没有烘焙动画或 UE 运行时动画。工程内柜门和抽屉恢复关闭。')
sheet([(OUT/p,t) for p,t in [
    ('02_Reverse.png','反向视角 · 临时隐藏遮挡墙体'),
    ('02_Top.png','俯视 · 保留用户修正后的占地'),
    ('Fireplace_Front.png','壁炉 · 内部退进与炉栅'),
    ('CoffeeTable_Front.png','茶几 · 框架、围板、腿')]],
    'Review_Structure.png','预览隐藏不修改墙体网格。相机为 Blender 检查视图，未声称完成 UE Gameplay Camera 验收。')
print('Four Level 2 review sheets saved.')
