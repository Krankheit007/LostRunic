"""Arrange unretouched original and Blender renders on labelled review sheets."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps

OUT = Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Stage01')
FONT = ImageFont.truetype('C:/Windows/Fonts/msyh.ttc', 26)
SMALL = ImageFont.truetype('C:/Windows/Fonts/msyh.ttc', 19)

def board(items, filename, columns, cell=(760, 660), footer=''):
    width, height = cell
    rows = (len(items)+columns-1)//columns
    canvas = Image.new('RGB', (columns*width, rows*height+60), '#eeece7')
    draw = ImageDraw.Draw(canvas)
    for index, (path, title) in enumerate(items):
        x, y = (index%columns)*width, (index//columns)*height
        pic = ImageOps.contain(Image.open(path).convert('RGB'), (width-20, height-60))
        canvas.paste(pic, (x+(width-pic.width)//2, y+50+(height-60-pic.height)//2))
        draw.text((x+16, y+10), title, font=FONT, fill='#292b2e')
    draw.text((16, rows*height+17), footer, font=SMALL, fill='#414349')
    canvas.save(OUT/filename)

board([
    (Path('D:/GameDesign/DontForgetAdele/livingroom.png'), '原始设计图 · 唯一造型依据'),
    (OUT/'01_Reference.png', '一级位置修正版 · Blender 实际渲染'),
], 'Review_Comparison.png', 2,
footer='只评审外轮廓、比例和高低关系；颜色为材质分区占位。一级尚待用户验收。')
board([(OUT/p, title) for p, title in [
    ('01_Reference.png', '近似参考视角'),
    ('02_Reverse.png', '反向视角 · 隐藏遮挡墙体'),
    ('03_Top.png', '俯视 · 尺寸与间距'),
    ('04_Front_Clay.png', '正面灰模 · 高低关系'),
]], 'Review_Views.png', 2, (660, 595),
footer='左侧与前侧墙体为验收剖切省略；这不是最终场景。')
board([(OUT/p, title) for p, title in [
    ('05_Sofa_Front.png', '沙发 · 正面（遮挡部分为推定）'),
    ('06_Sofa_Back.png', '沙发 · 背面'),
    ('07_ArmchairOchre.png', '赭黄扶手椅 · 主体轮廓'),
    ('08_ArmchairRusset.png', '红褐扶手椅 · 主体轮廓'),
]], 'Review_Seating.png', 2, (660, 595),
footer='后续二级结构包括坐垫、靠垫、腿部曲线、卷扶手与布料垂搭；本阶段未展开。')
board([(OUT/('Silhouette_'+name+'_'+view+'.png'), label) for name, view, label in [
    ('Sofa', 'Front', '沙发 · 正面'), ('Sofa', 'Side', '沙发 · 侧面'),
    ('ArmchairOchre', 'Front', '赭黄扶手椅'), ('ArmchairRusset', 'Front', '红褐扶手椅'),
    ('CoffeeTable', 'Front', '茶几'), ('ArchBookcase', 'Front', '拱顶书柜'),
    ('Fireplace', 'Front', '壁炉包络'), ('WindowConsole', 'Front', '窗下长桌'),
    ('Ottoman', 'Front', '脚凳'), ('WoodChair', 'Front', '木椅'),
    ('SmallCabinet', 'Front', '小柜'), ('SideTableCup', 'Front', '配套圆桌'),
]], 'Review_BlackSilhouette.png', 4, (430, 395),
footer='LEVEL 1 — 黑色剪影：独立正交视图，检查包络、比例和负空间；各格独立取景，不用于比较绝对尺寸。')
board([
    (OUT/'History/BeforeLayoutCorrection/01_Reference.png', '上版 · 用户指出位置偏差'),
    (OUT/'01_Reference.png', '本版 · 位置与遮挡修正'),
], 'Review_LayoutBeforeAfter.png', 2,
footer='本版略扩大概念取景以完整显示窗左侧挂画；家具本体尺寸不变。请结合俯视和原图对照验收。')
print('Five review sheets saved.')
