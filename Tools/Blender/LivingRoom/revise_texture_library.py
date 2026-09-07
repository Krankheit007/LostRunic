"""Source-directed aged material textures, deterministic multiscale variation."""
from pathlib import Path
import json,re,shutil
import numpy as np
from PIL import Image
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
OUT=BASE/'SurfaceRevision/Textures'
old=json.loads((BASE/'Delivery/TextureManifest.json').read_text())
rng=np.random.default_rng(739)
N=1024
def noise(size):
    a=rng.random((size,size)).astype('float32')
    return np.asarray(Image.fromarray(a).resize((N,N),Image.Resampling.BICUBIC))-.5
y,x=np.mgrid[0:N,0:N]/N
palette={'FloorMass':(88,80,73),'CoolPlaster':(103,112,121),'Wood':(77,48,28),
 'WoodPanel':(89,57,34),'DetailWood':(87,57,33),'OliveCloth':(76,79,45),
 'OchreCloth':(162,112,33),'RussetCloth':(105,49,28),'TerracottaThrow':(118,67,55),
 'CreamCloth':(181,170,146),'CurtainMass':(194,182,153),'WarmStone':(169,162,145),
 'Interior':(43,33,25),'DarkRecess':(31,28,25),'Iron':(46,44,40),
 'AgedBrass':(130,91,38),'DetailAgedBrass':(121,87,40),'PotMass':(94,80,58),
 'Leaf':(52,68,32),'LeafLight':(79,91,42),'FoliageMass':(48,66,33)}
made={}
for original,item in old.items():
    key=item['material'].removeprefix('M_LR_')
    if key in made:continue
    coarse=noise(9);medium=noise(45);fine=noise(256)
    var=coarse*20+medium*9+fine*3
    if key in palette:color=np.array(palette[key],float)
    else:
        src=np.asarray(Image.open(BASE/'Delivery/Textures'/item['BC']).convert('RGB'))
        color=src.mean(axis=(0,1))*.77
    rough=np.full((N,N),.79);height=medium*.08;g=np.zeros((N,N));r=np.full((N,N),.18)
    if 'Wood' in key or 'Floor' in key:
        warp=.02*np.sin(x*9)+.04*coarse
        grain=np.sin((y+warp)*240)+.5*np.sin((y+warp)*571)
        var=coarse*18+medium*5+grain*1.5+fine*3
        height=grain*.025+fine*.01;rough=.53+coarse*.16+medium*.1;r[:]=.38
    if key=='FloorMass':
        row=np.floor(y*5)
        edges=np.mod(y*5,1)
        seam=(edges<.018)|(np.mod(x+(row%3)/3,1)<.006)
        var+=np.sin(row*5.7)*9-seam*21
        height-=seam*.25;g=seam*.42;rough=.73+coarse*.14
    if key=='CoolPlaster':
        chips=np.maximum(0,medium+coarse*.45-.06)
        var=coarse*24+medium*12-chips*23
        color=np.array(palette[key]);height=chips*.2+fine*.015
    if 'Cloth' in key or key in ('TerracottaThrow','CurtainMass'):
        weave=np.sin(x*2*np.pi*240)*np.sin(y*2*np.pi*240)
        var=coarse*10+medium*5+weave*3
        height=weave*.015+fine*.014;rough=.88+coarse*.1
    if 'Brass' in key:
        var=coarse*36+medium*13;rough=.49+coarse*.30
    rgb=np.clip(color+var[:,:,None],0,255).astype('uint8')
    if key=='RugMass':
        bc='T_LR_Rug_BC.png';rough[:]=.94
    else:
        bc=f'T_LR_{key}_BC.png';Image.fromarray(rgb).save(OUT/bc)
    smk=f'T_LR_{key}_SMK.png'
    Image.fromarray((np.clip(np.stack([r,g,.5+coarse*.5],-1),0,1)*255).astype('uint8')).save(OUT/smk)
    roughfile=f'T_LR_{key}_Roughness.png'
    Image.fromarray((np.clip(rough,0,1)*255).astype('uint8')).save(OUT/roughfile)
    # Exported normal uses DirectX green; Blender preview inverts green explicitly.
    dy,dx=np.gradient(height)
    normal=np.stack([-dx*6,dy*6,np.ones_like(dx)],-1)
    normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
    norm=f'T_LR_{key}_Normal.png'
    Image.fromarray(((normal*.5+.5)*255).astype('uint8')).save(OUT/norm)
    made[key]={**item,'BC':bc,'SMK':smk,'Roughness':roughfile,'Normal':norm,
       'normal':'DirectX tangent normal; preview green inverted','resolution':N}
mapping={k:made[v['material'].removeprefix('M_LR_')] for k,v in old.items()}
(OUT.parent/'TextureManifest.json').write_text(json.dumps(mapping,indent=2),encoding='utf-8')
shutil.copy2('D:/GameDesign/DontForgetAdele/livingroom.png',OUT/'ConceptArtworkSource.png')
print('32 material sets revised')
