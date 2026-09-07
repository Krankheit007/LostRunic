"""Source-specific seating structure, preserving accepted outer backs and layout."""
import bpy
import math
from structure_common import *

def rolled_arm(parent,old,mat):
    x,y,z=old.location
    w,d,h=old.dimensions
    vertices=[]
    n=20
    layers=[(-.5,.73),(-.46,1),(-.25,.98),(.30,.90),(.46,.99),(.50,.76)]
    for dy,f in layers:
        for j in range(n):
            a=2*math.pi*j/n
            vertices.append((x+w*.5*f*math.cos(a),y+d*dy,z+h*.5*f*math.sin(a)))
    faces=[tuple(reversed(range(n)))]
    for k in range(len(layers)-1):
        for j in range(n):
            a=k*n+j;b=k*n+(j+1)%n
            faces.append((a,b,b+n,a+n))
    faces.append(tuple((len(layers)-1)*n+j for j in range(n)))
    mesh(parent,'RolledArm',vertices,faces,mat)

def seating(name,seats):
    p=root(name)
    old=children(p,'SeatEnvelope')[0]
    mat=old.data.materials[0]
    width,depth,_=old.dimensions
    box(p,'LowerUpholsteredFrame',(0,0,.275),(width,depth,.31),mat,.065)
    usable=width-.20
    for i in range(seats):
        x=(i-(seats-1)/2)*(usable/seats)
        puff(p,'SeatCushion',(x,.055,.465),(usable/seats-.018,depth-.19,.17),mat)
        cushion=puff(p,'BackCushion',(x,-depth/2+.19,.80),
            (usable/seats-.03,.17,.43 if seats>1 else .49),mat)
        cushion.rotation_euler.x=math.radians(-8)
    remove([old])
    for arm in children(p,'RolledArmEnvelope'):
        rolled_arm(p,arm,mat)
        remove([arm])
    for oldleg in children(p,'SupportMass'):
        shaped_leg(p,oldleg.location[:2],oldleg.dimensions.z,.033,.008)
        remove([oldleg])
    if seats==2:
        for x,zrot in [(-.57,17),(.57,-16)]:
            obj=puff(p,'LooseCushion',(x,-.035,.745),(.43,.14,.36),CREAM)
            obj.rotation_euler=(math.radians(-15),0,math.radians(zrot))
        throw(p)
    else:
        obj=puff(p,'LooseCushion',(0,-.045,.73),(.34,.13,.32),CREAM)
        obj.rotation_euler=(math.radians(-18),0,math.radians(-12))
        if name=='ArmchairOchre':
            obj=puff(p,'SmallCushion',(.12,.16,.62),(.25,.13,.14),CREAM)
            obj.rotation_euler.z=math.radians(16)
    p['structure']='Frame, roll arms, shaped legs, separate seat/back cushions, loose cushions'

def throw(p):
    path=[(.30,.56),(.08,.57),(-.08,.66),(-.08,.85),(-.08,1.04),
          (-.20,1.12),(-.40,1.10),(-.51,.99),(-.54,.75),(-.55,.47),(-.55,.21)]
    vertices=[]
    nx=20
    for k,(y,z) in enumerate(path):
        for i in range(nx+1):
            u=i/nx
            wave=.013*math.sin(u*6*math.pi+.3*k)
            vertices.append((-.30+(u-.5)*.57,y+(wave if k>5 else 0),z+(wave if k<=5 else .007*math.sin(u*math.pi))))
    faces=[]
    for k in range(len(path)-1):
        for i in range(nx):
            a=k*(nx+1)+i
            faces.append((a,a+1,a+nx+2,a+nx+1))
    obj=mesh(p,'DrapedThrow',vertices,faces,THROW)
    obj['shell']='open cloth surface; thin Solidify in preview'
    sub=obj.modifiers.new('MacroDrape','SUBSURF');sub.levels=2
    mod=obj.modifiers.new('ClothThickness','SOLIDIFY');mod.thickness=.005

def ottoman():
    p=root('Ottoman')
    old=children(p,'UpholsteredEnvelope')[0]
    m=old.data.materials[0]
    remove([old])
    box(p,'Base',(0,0,.265),(.72,.65,.32),m,.055)
    for x in (-.18,.18):
        for y in (-.1625,.1625):
            puff(p,'TopCushion',(x,y,.473),(.354,.319,.125),m)

def build():
    seating('Sofa',2)
    seating('ArmchairOchre',1)
    seating('ArmchairRusset',1)
    ottoman()
