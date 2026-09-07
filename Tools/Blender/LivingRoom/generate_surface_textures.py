"""Deterministic low-frequency BC / project-semantic SMK material library."""
from pathlib import Path
import json,re
import numpy as np
from PIL import Image
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Delivery')
OUT=BASE/'Textures';OUT.mkdir(exist_ok=True)
sources=json.loads((BASE/'material_source.json').read_text())
mapping={}
for original,rgba in sources.items():
    name=re.sub(r'\.\d+$','',original)
    key=re.sub(r'^S0[12]_','',name)
    n=1024 if any(t in key for t in ('Floor','Rug')) else 512
    yy,xx=np.mgrid[0:n,0:n].astype(float);u=xx/n;v=yy/n
    color=np.array(rgba[:3]);color=np.where(color<=.0031308,color*12.92,1.055*color**(1/2.4)-.055)*255
    if 'Floor' in key:color=np.array([109,104,96.])
    if 'Plaster' in key:color=np.array([137,143,146.])
    if 'Rug' in key:color=np.array([193,171,139.])
    macro=(np.sin(2*np.pi*u)*np.sin(2*np.pi*v)+1)/2
    variation=(macro-.5)*5
    cavity=np.zeros((n,n));detail=np.full((n,n),.10)
    if 'Wood' in key or 'Floor' in key:
        variation+=2*np.sin(2*np.pi*(v*12+.13*np.sin(u*2*np.pi)))
        detail[:]=.24
    if 'Floor' in key:
        row=np.floor(v*5)
        seam=(np.mod(v*5,1)<.016)|(np.mod(u+np.mod(row,2)*.5,1)<.006)
        cavity=seam.astype(float)*.32
        variation+=np.sin(row*2.5)*5-seam*15
    rgb=np.clip(color[None,None,:]+variation[:,:,None],0,255)
    if 'Rug' in key:
        edge=np.minimum.reduce([u,1-u,v,1-v])
        border=((edge>.035)&(edge<.055))|((edge>.12)&(edge<.13))
        rgb[border]=[152,111,81]
        # Coarse border rhythm follows the source's faded warm decorative band.
        band=(edge>.065)&(edge<.11)
        motif=(np.sin(u*24*np.pi)*np.sin(v*32*np.pi)>.2)&band
        rgb[motif]=[170,131,96]
    bc=OUT/f'T_LR_{key}_BC.png';smk=OUT/f'T_LR_{key}_SMK.png'
    Image.fromarray(rgb.astype('uint8'),'RGB').save(bc)
    mask=np.stack([detail,cavity,.35+.15*macro],axis=-1)
    Image.fromarray((mask*255).astype('uint8'),'RGB').save(smk)
    metal=1.0 if ('Brass' in key or key=='Iron') else 0.0
    roughness=.45 if metal or 'Porcelain' in key else .82
    mapping[original]={'material':'M_LR_'+key,'BC':bc.name,'SMK':smk.name,
       'roughness':roughness,'metallic':metal,'resolution':n,
       'SMK_semantics':'R DetailAmount; G artist-defined floor board seams only, otherwise zero; B MacroVariation; no Alpha',
       'normal':'omitted: no justified medium-scale extra normal detail'}
(BASE/'TextureManifest.json').write_text(json.dumps(mapping,indent=2),encoding='utf-8')
print('Generated',len(set(v['BC'] for v in mapping.values())),'BC/SMK pairs')
