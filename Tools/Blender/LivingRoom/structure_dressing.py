"""Visible large drapery, hearth and foliage structure; no fine ornament."""
import math
from structure_common import *

def curtains():
    p=root('Curtains')
    for old in children(p,'DraperyEnvelope'):
        cx,cy,cz=old.location
        w,d,h=old.dimensions
        mat=old.data.materials[0]
        vertices=[];faces=[]
        nx,nz=28,18
        for j in range(nz+1):
            v=j/nz
            for i in range(nx+1):
                u=i/nx
                x=cx+(u-.5)*w*(1-.065*math.sin(math.pi*v))
                y=cy+.038*math.cos(u*6*math.pi)+.015*math.sin(v*math.pi)
                z=cz+(v-.5)*h+.009*math.sin(u*6*math.pi)*(1-v)
                vertices.append((x,y,z))
        for j in range(nz):
            for i in range(nx):
                a=j*(nx+1)+i
                faces.append((a,a+1,a+nx+2,a+nx+1))
        obj=mesh(p,'CurtainBroadFolds',vertices,faces,mat)
        obj['shell']='open cloth surface; thin Solidify in preview'
        mod=obj.modifiers.new('FabricThickness','SOLIDIFY');mod.thickness=.006
        remove([old])

def fireplace():
    p=root('Fireplace')
    # The user's chimney wall now starts in front of the old recess backing.
    # Bring only the backing forward inside the accepted fireplace envelope.
    children(p,'FireboxRecess')[0].location.y=-.06
    stone=children(p,'PierEnvelope')[0].data.materials[0]
    for sign in (-1,1):
        box(p,'InnerFireboxSide',(sign*.535,-.075,.65),(.04,.25,1.04),DARK)
        box(p,'PierInset',(sign*.68,.179,.75),(.16,.018,1.04),stone,.008)
    box(p,'FireboxFloor',(0,0,.145),(1.08,.30,.06),DARK)
    for x in (-.38,.38):
        beam(p,'GrateFoot',(x,.19,.15),(x,.19,.35),.016,IRON)
    for z in (.23,.34):
        beam(p,'GrateRail',(-.46,.19,z),(.46,.19,z),.012,IRON)
    for x in (-.40,-.24,-.08,.08,.24,.40):
        beam(p,'GrateUpright',(x,.19,.20),(x,.19,.42),.010,IRON)
    for k in range(3):
        beam(p,'HearthLog',(-.34,-.06+k*.075,.21+k*.021),(.34,.01+k*.04,.23+k*.021),.042,WOOD)

def baskets():
    p=root('LogBasket')
    remove(list(p.children))
    box(p,'BasketFloor',(0,0,.04),(.42,.35,.07),WOOD)
    for sign in (-1,1):
        box(p,'BasketSide',(sign*.195,0,.22),(.03,.35,.36),WOOD)
        box(p,'BasketEnd',(0,sign*.16,.22),(.39,.03,.36),WOOD)
    for k in range(5):
        x=(k-2)*.07
        beam(p,'CutLog',(x,-.11,.13),(x+.016,.09,.43+(k%2)*.04),.044,WOOD_LIGHT)
    p=root('FireTools')
    remove(list(p.children))
    box(p,'StandFoot',(0,0,.025),(.31,.25,.05),IRON)
    beam(p,'Stand',(0,0,.03),(0,0,.50),.016,IRON)
    beam(p,'TopRack',(-.125,0,.48),(.125,0,.48),.014,IRON)
    for k in range(3):
        x=(k-1)*.10
        beam(p,'ToolShaft',(x,.025,.11),(x,.025,.48),.008,IRON)
        box(p,'ToolHead',(x,.025,.08),(.055,.026,.10),IRON,.003)

def leaf(parent,name,start,end,width,mat):
    a,b=Vector(start),Vector(end)
    v=b-a
    side=v.cross(Vector((0,0,1)))
    if side.length<.001:
        side=Vector((1,0,0))
    side.normalize()
    center=a+v*.48
    raised=center+Vector((0,0,width*.16))
    verts=[a,a+v*.26+side*width*.70,center+side*width,
        b,center-side*width,a+v*.26-side*width*.70,raised]
    obj=mesh(parent,name,verts,[(i,(i+1)%6,6) for i in range(6)],mat)
    obj['shell']='open two-sided leaf surface; no closed-shell claim'
    return obj

def plant(name,palm=False):
    p=root(name)
    pot=children(p,'PotEnvelope')[0]
    size=pot.dimensions.z/.30
    base=pot.location.z-.15*size
    remove(children(p,'CanopyEnvelope')+children(p,'StemEnvelope'))
    beam(p,'Stem',(0,0,base+.20*size),(0,0,base+.71*size),.012*size,WOOD)
    if palm:
        for k in range(9):
            a=2*math.pi*k/9
            radial=Vector((math.cos(a),math.sin(a),0))
            tangent=Vector((-math.sin(a),math.cos(a),0))
            points=[]
            for j in range(9):
                t=j/8
                xyz=radial*(.44*size*t)
                xyz.z=base+size*(.65+.46*math.sin(t*math.pi*.76))
                points.append(xyz)
            for j in range(8):
                beam(p,'FrondRachis',points[j],points[j+1],.0035*size,LEAF)
            for j in range(2,9):
                t=j/9
                for sign in (-1,1):
                    tip=points[j]+tangent*(sign*.13*size*(1-t*.60))+radial*(.04*size)
                    tip.z-=.025*size
                    leaf(p,'PalmLeaflet',points[j],tip,.021*size,LEAF if k%2 else LEAF_LIGHT)
    else:
        for k in range(15):
            a=k*2.39996
            radial=Vector((math.cos(a),math.sin(a),0))
            z=base+size*(.45+(k%5)*.105)
            start=Vector((0,0,z))
            joint=start+radial*(.09*size)
            joint.z+=.045*size
            tip=joint+radial*(.16*size)
            tip.z+=size*(.12 if k%2 else .19)
            beam(p,'LeafStem',start,joint,.003*size,WOOD)
            leaf(p,'BroadLeaf',joint,tip,.063*size,LEAF if k%2 else LEAF_LIGHT)

def books():
    palette=[material('BookBlue',(.13,.18,.21)),material('BookOchre',(.31,.24,.13)),
        material('BookOlive',(.19,.22,.16)),material('BookRusset',(.28,.13,.085))]
    # Large visible contents only: no spine labels or invented ornament.
    p=root('ArchBookcase')
    for shelf in (0,1,2):
        for i in range(4):
            x=-.26+i*.082+(shelf%2)*.07
            h=.22+(i%3)*.026
            obj=box(p,'BookVolume',(x,-.025,1.02+shelf*.39+h/2),(.062,.15,h),palette[(i+shelf)%4],.003)
            obj['export_group']='BookcaseContents'
    for name,base,offset in [('CoffeeTable',.48,(.30,-.01)),('WindowConsole',.83,(.18,0)),('Ottoman',.535,(0,0))]:
        p=root(name)
        for i in range(3):
            obj=box(p,'BookStack',(offset[0],offset[1],base+.018+i*.037),(.27-i*.018,.20,.032),palette[i],.003)
            obj.rotation_euler.z=math.radians((i-1)*5)
            obj['export_group']='LooseBooks'

def door_frame():
    scene=bpy.context.scene
    p=bpy.data.objects.new('ROOT_BackDoor',None)
    scene.collection.objects.link(p)
    p.location=(-2.5,3.01,0)
    p.rotation_euler.z=math.pi
    p['category']='ArchitectureAddition'
    for sign in (-1,1):
        box(p,'DoorJamb',(sign*.51,0,1.18),(.09,.16,2.36),WOOD_LIGHT)
    box(p,'DoorHead',(0,0,2.34),(1.11,.16,.10),WOOD_LIGHT)
    hinge=pivot(p,'DoorLeaf',(-.448,.03,0),'HINGE','Z',-85)
    panel=box(p,'DoorLeaf',(-.006,.03,1.155),(.876,.05,2.29),WOOD)
    reparent_keep_local(panel,hinge)
    for z,h in ((.60,.88),(1.69,.86)):
        piece=box(p,'DoorPanel',(-.006,.062,z),(.65,.022,h),WOOD_LIGHT,.01)
        reparent_keep_local(piece,hinge)
    hinge.rotation_euler.z=math.radians(-65)
    hinge['preview_state']='Open 65 degrees as concept; hinge side/interior inferred; no animation'
    p['structure']='New frame fits existing approved doorway; approved wall meshes untouched'

def build():
    curtains()
    fireplace()
    baskets()
    for name in ('Palm','ForegroundPlant','ConsolePlant','CabinetPlant'):
        plant(name,palm=name=='Palm')
    books()
    door_frame()
