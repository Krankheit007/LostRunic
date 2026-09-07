"""Complete source-visible decorative assets at Level 2, not surface ornament.

Each PROP empty is an independent export unit, parented to its support only for
assembly. No furniture root or accepted architecture is transformed here.
"""
import bpy
import math
from mathutils import Vector
from structure_common import *
from structure_dressing import leaf

CERAMIC=material('VaseCeladon',(.32,.40,.37))
PORCELAIN=material('IvoryPorcelain',(.72,.67,.55))
BRASS=material('AgedBrass',(.34,.245,.105))
PAPER=material('BookPageBlock',(.55,.49,.38))
FLOWER=material('IvoryFlowerMass',(.72,.70,.51))
LIQUID=material('TeaSurface',(.065,.035,.014))

def prop(support,name,loc,description,kind='dressing'):
    obj=bpy.data.objects.new('PROP_'+name,None)
    support.users_collection[0].objects.link(obj)
    obj.parent=support
    obj.location=loc
    obj['independent_asset']=True
    obj['completion_kind']=kind
    obj['source_visible_structure']=description
    obj['export_contract']='Independent static prop; local placement inherited from support; no animation'
    return obj

def revolve(parent,name,profile,mat,segments=24):
    # Radius/height profile travels from outer bottom up the outer wall and
    # back down the inner wall. Single-vertex axis ends avoid degenerate rings.
    verts=[];rings=[]
    for r,z in profile:
        if r==0:
            rings.append([len(verts)]);verts.append((0,0,z))
        else:
            ring=[]
            for j in range(segments):
                a=2*math.pi*j/segments
                ring.append(len(verts));verts.append((r*math.cos(a),r*math.sin(a),z))
            rings.append(ring)
    faces=[]
    for a,b in zip(rings,rings[1:]):
        for j in range(segments):
            k=(j+1)%segments
            if len(a)==1:
                faces.append((a[0],b[k],b[j]))
            elif len(b)==1:
                faces.append((a[j],a[k],b[0]))
            else:
                faces.append((a[j],a[k],b[k],b[j]))
    return mesh(parent,name,verts,faces,mat)

def vase(parent,height=.20,radius=.061):
    h,r=height,radius
    profile=[(0,0),(r*.62,0),(r*.77,h*.07),(r,h*.37),
        (r*.91,h*.66),(r*.48,h*.85),(r*.46,h),
        (r*.35,h),(r*.37,h*.86),(r*.76,h*.62),(r*.80,h*.30),
        (r*.58,h*.075),(0,h*.075)]
    return revolve(parent,'HollowVase',profile,CERAMIC)

def bouquet(parent,base,spread=.19,height=.32,count=11):
    for i in range(count):
        a=i*2.399963
        reach=spread*(.55+.45*((i*7)%11)/10)
        end=Vector((math.cos(a)*reach,math.sin(a)*reach*.58,
            base+height*(.64+.36*((i*3)%11)/10)))
        start=Vector((.008*math.cos(a),.008*math.sin(a),base-.055))
        bend=start+(end-start)*.54+Vector((.009*math.sin(a),0,.025))
        beam(parent,'Branch',start,bend,.0025,LEAF)
        beam(parent,'Branch',bend,end,.0017,LEAF)
        side=Vector((-math.sin(a),math.cos(a),.22))
        for j,t in enumerate((.42,.65,.83)):
            at=start.lerp(end,t)
            tip=at+side*((-1 if j%2 else 1)*.050)+Vector((0,0,.027))
            leaf(parent,'BouquetLeaf',at,tip,.018,LEAF_LIGHT if i%3 else LEAF)
        if i%2==0:
            # A single coarse corolla mass; petals and flower species are not inferred.
            bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,location=end)
            obj=bpy.context.object
            for v in obj.data.vertices:
                v.co.x*=.016;v.co.y*=.014;v.co.z*=.011
            attach(obj,parent,'FlowerMass',FLOWER)
            obj['structure_note']='Coarse flower silhouette only; no small petal detail'

def floral(support,name,xy,z,h=.20,r=.058,spread=.18,flower_h=.30):
    p=prop(support,name,(*xy,z),'Visible vase with branching bouquet; species and back branches inferred')
    vase(p,h,r)
    bouquet(p,h*.90,spread,flower_h,count=17 if name=='MantelBouquet' else 13)
    return p

def bowl(parent,r=.08,h=.042):
    return revolve(parent,'HollowBowl',[(0,0),(r*.40,0),(r*.47,.009),
        (r*.84,h*.60),(r,h),(r*.91,h),(r*.74,h*.56),
        (r*.35,.014),(0,.014)],BRASS)

def candlestick(support,name,x,z,height):
    p=prop(support,name,(x,.12,z),'Visible slender candlestick and candle; no engraved decoration')
    revolve(p,'Holder',[(0,0),(.038,0),(.042,.008),(.031,.018),
        (.014,.034),(.009,height*.65),(.017,height*.73),
        (.022,height*.79),(.022,height*.83),(0,height*.83)],BRASS)
    beam(p,'Candle',(0,0,height*.80),(0,0,height),.009,PORCELAIN)

def cup_set(support):
    p=prop(support,'TeaCupAndSaucer',(0,0,.56),
        'Visible cup, open mouth, curved handle, saucer and dark liquid surface')
    revolve(p,'Saucer',[(0,0),(.064,0),(.089,.008),(.09,.014),
        (.079,.019),(.057,.010),(0,.010)],PORCELAIN,32)
    cup=revolve(p,'Cup',[(0,.010),(.030,.010),(.035,.018),(.045,.036),
        (.053,.085),(.053,.090),(.047,.090),(.046,.084),
        (.037,.034),(.026,.020),(0,.020)],PORCELAIN,32)
    # C-shaped handle in a vertical plane, closed cross-section and end caps.
    verts=[];faces=[];n=18;m=8
    for i in range(n+1):
        a=-math.pi/2+i*math.pi/n
        for j in range(m):
            b=2*math.pi*j/m
            verts.append((.046+(.027+.005*math.cos(b))*math.cos(a),
                .005*math.sin(b),.052+(.027+.005*math.cos(b))*math.sin(a)))
    faces.append(tuple(reversed(range(m))))
    for i in range(n):
        for j in range(m):
            a=i*m+j;b=i*m+(j+1)%m
            faces.append((a,b,b+m,a+m))
    faces.append(tuple(n*m+j for j in range(m)))
    mesh(p,'CupHandle',verts,faces,PORCELAIN)
    revolve(p,'Tea',[(0,.074),(.0435,.074),(.0435,.076),(0,.076)],LIQUID,32)

def refine_books():
    candidates=[o for o in bpy.context.scene.objects if o.type=='MESH' and
        ('_BookVolume' in o.name or '_BookStack' in o.name) and not o.parent.get('independent_asset')]
    for index,old in enumerate(candidates):
        upright='_BookVolume' in old.name
        support=old.parent
        p=prop(support,support.name.removeprefix('ROOT_')+'_Book_'+str(index+1),
            old.location,'Separate front/back covers, page block and spine; no printed detail','book_structure')
        p.rotation_euler=old.rotation_euler.copy()
        w,d,h=old.dimensions
        m=old.data.materials[0]
        if upright:
            box(p,'Pages',(0,-.003,0),(w-.010,d-.012,h-.012),PAPER,.001)
            for sign in (-1,1):
                box(p,'Cover',(sign*(w/2-.002),0,0),(.004,d,h),m,.001)
            box(p,'Spine',(0,d/2-.003,0),(w-.008,.006,h),m,.001)
        else:
            box(p,'Pages',(0,.003,0),(w-.010,d-.012,h-.010),PAPER,.001)
            for sign in (-1,1):
                box(p,'Cover',(0,0,sign*(h/2-.002)),(w,d,.004),m,.001)
            box(p,'Spine',(0,-d/2+.003,0),(w,.006,h-.008),m,.001)
        remove([old])

def build():
    # Idempotent completion on the current Stage02 file; keep completed books.
    old_props=[p for p in bpy.context.scene.objects if p.get('completion_kind')=='dressing']
    for p in old_props:
        remove(list(p.children_recursive));remove([p])
    mantel=root('Fireplace')
    for i,(x,h) in enumerate(((.66,.34),(.49,.40),(.34,.30))):
        candlestick(mantel,'MantelCandlestick'+str(i+1),x,1.56,h)
    floral(mantel,'MantelBouquet',(-.36,.17),1.56,.21,.063,.23,.22)
    p=prop(mantel,'MantelSmallVessel',(-.67,.14,1.56),'Small secondary vessel visible beside bouquet')
    vase(p,.145,.035)
    p=prop(mantel,'MantelLowOrnament',(-.02,.16,1.56),'Low rounded tabletop mass; exact purpose unclear in concept')
    bowl(p,.044,.023)
    table=root('CoffeeTable')
    floral(table,'CoffeeTableBouquet',(-.28,-.035),.48,.19,.053,.16,.30)
    p=prop(table,'CoffeeTableBowl',(.08,.18,.48),'Visible shallow dark bowl')
    bowl(p,.082,.043)
    p=prop(table,'CoffeeTableSmallVessel',(-.36,.205,.48),'Small vessel on flat paper-like base; purpose uncertain')
    box(p,'FlatBase',(0,0,.004),(.125,.080,.008),PAPER,.001)
    vessel=revolve(p,'SmallVessel',[(0,.008),(.022,.008),(.027,.017),(.023,.056),
        (.013,.065),(.013,.078),(0,.078)],CERAMIC)
    cup_set(root('SideTableCup'))
    floral(root('SideTablePlant'),'SideTableBouquet',(-.025,0),.70,.18,.052,.15,.25)
    refine_books()
    bpy.context.view_layer.update()
    print('Tabletop prop structures complete; existing furniture roots unchanged.')
