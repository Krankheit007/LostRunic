"""Living-room cabinets and tables, using the approved transforms unchanged."""
import math
from structure_common import *

def panel_door(parent,name,x,sign,z,width,height,y):
    hinge=pivot(parent,name,(x+sign*width/2,y,z),'HINGE','Z',-90*sign)
    pieces=[]
    pieces.append(box(parent,name+'_Panel',(x,y-.011,z),(width-.025,.028,height-.025),WOOD_LIGHT))
    for sx in (-1,1):
        pieces.append(box(parent,name+'_Stile',(x+sx*(width/2-.025),y+.006,z),(.045,.037,height),WOOD))
    for sz in (-1,1):
        pieces.append(box(parent,name+'_Rail',(x,y+.006,z+sz*(height/2-.035)),(width-.08,.037,.06),WOOD))
    for obj in pieces:
        reparent_keep_local(obj,hinge)
    return hinge

def cabinet_shell(p,width,depth,z0,z1):
    for sign in (-1,1):
        box(p,'CarcassSide',(sign*(width/2-.025),0,(z0+z1)/2),(.05,depth,z1-z0-.08))
    box(p,'CarcassBack',(0,-depth/2+.015,(z0+z1)/2),(width-.10,.03,z1-z0-.08),DARK)
    for z in (z0+.02,z1-.02):
        box(p,'CarcassHorizontal',(0,0,z),(width,depth,.04))

def bookcase():
    p=root('ArchBookcase')
    remove(list(p.children))
    cabinet_shell(p,.80,.38,.10,.98)
    box(p,'Plinth',(0,0,.05),(.80,.38,.10))
    box(p,'CabinetShelf',(0,0,.48),(.70,.32,.035),DARK)
    for x,sign in [(-.188,-1),(.188,1)]:
        panel_door(p,'DoorLeft' if sign<0 else 'DoorRight',x,sign,.50,.368,.77,.176)
    # Upper case: thin arched back retains the accepted silhouette, open face toward +Y.
    points=[(-.347,.98),(.347,.98)]
    points += [(.347*math.cos(i*math.pi/20),2.19+.347*math.sin(i*math.pi/20)) for i in range(21)]
    n=len(points)
    vertices=[(x,y,z) for y in (-.16,-.135) for x,z in points]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh(p,'ArchedBack',vertices,faces,DARK,False)
    for sign in (-1,1):
        box(p,'UpperStile',(sign*.374,-.047,1.58),(.052,.27,1.24))
    vertices=[]
    for i in range(25):
        a=i*math.pi/24
        for radius,y in [(.40,-.185),(.40,.085),(.347,.085),(.347,-.185)]:
            vertices.append((radius*math.cos(a),y,2.19+radius*math.sin(a)))
    faces=[(3,2,1,0)]
    for i in range(24):
        for j in range(4):
            a=i*4+j;b=i*4+(j+1)%4
            faces.append((a,b,b+4,a+4))
    faces.append(tuple(24*4+j for j in range(4)))
    rounding(mesh(p,'ArchedFrame',vertices,faces,WOOD,False),.003)
    for z in (.99,1.37,1.76,2.15):
        box(p,'Shelf',(0,-.038,z),(.70,.247,.032))
    p['structure']='Arched open shelving; two inset doors and side-edge hinge pivots; closed pose'

def small_cabinet():
    p=root('SmallCabinet')
    remove(children(p,'CabinetEnvelope'))
    cabinet_shell(p,.70,.46,.09,.83)
    for i,z in enumerate((.225,.455,.685)):
        box(p,'DrawerRail',(0,.19,z-.117),(.60,.065,.035))
        drawer(p,'Drawer'+str(i+1),0,z,.588,.37,.197,.215)
    p['structure']='Three drawer interpretation from partly visible facade; hidden interior inferred'

def rectangular_table(name):
    p=root(name)
    top=children(p,'TopEnvelope')[0]
    w,d,_=top.dimensions
    apron=children(p,'ApronEnvelope')[0]
    az=apron.location.z
    ah=apron.dimensions.z
    remove([apron])
    for sign in (-1,1):
        box(p,'SideApron',(sign*(w/2-.07),0,az),(.055,d-.13,ah))
        if name!='WindowConsole' or sign<0:
            box(p,'LongApron',(0,sign*(d/2-.065),az),(w-.13,.045,ah))
    for leg in children(p,'SupportMass'):
        loc=leg.location[:2]
        height=leg.dimensions.z
        shaped_leg(p,loc,height,.033,.015)
        remove([leg])
    if name=='WindowConsole':
        console_front_opening(p)
        # Face is toward local +Y, away from the bay window.
        for i,x in enumerate((-.55,0,.55)):
            drawer(p,'Drawer'+str(i+1),x,.708,.517,.30,.105,.205)
        for y in (-.155,.155):
            beam(p,'LowerStretcher',(-.805,y,.18),(.805,y,.18),.017)
        for x in (-.805,.805):
            beam(p,'EndStretcher',(x,-.155,.18),(x,.155,.18),.018)
    p['structure']='Separate tabletop, hollow apron frame and shaped legs'

def console_front_opening(p):
    # Actual drawer apertures, rather than drawer boxes intersecting a solid apron.
    for z in (.65,.767):
        box(p,'DrawerFrameRail',(0,.175,z),(1.65,.045,.014))
    for x in (-.825,-.275,.275,.825):
        box(p,'DrawerDivider',(x,.015,.708),(.024,.36,.103))

def round_table(name):
    p=root(name)
    top=children(p,'RoundTopEnvelope')[0]
    z=top.location.z
    for leg in children(p,'LegEnvelope'):
        shaped_leg(p,leg.location[:2],leg.dimensions.z,.027,.012)
        remove([leg])
    # Structural apron below the circular top, no decorative beading yet.
    radius=top.dimensions.x/2-.012
    loft(p,'CircularApron',[(0,0,z-.10,radius*.94,radius*.94),
        (0,0,z-.025,radius,radius)],sides=32)

def wood_chair():
    p=root('WoodChair')
    remove(children(p,'BackEnvelope'))
    box(p,'BackTopRail',(0,-.20,.983),(.41,.057,.064),WOOD,.015)
    box(p,'BackBottomRail',(0,-.20,.75),(.40,.043,.042))
    for x in (-.12,0,.12):
        obj=box(p,'BackSplat',(x,-.20,.872),(.038,.035,.21),WOOD,.011)
        obj.rotation_euler.y=math.radians(-x*20)
    for sign in (-1,1):
        box(p,'SeatSideRail',(sign*.20,0,.412),(.045,.42,.065))
        box(p,'SeatFrontRail',(0,sign*.20,.412),(.40,.045,.065))
    for leg in children(p,'SupportMass'):
        shaped_leg(p,leg.location[:2],leg.dimensions.z,.024,.014)
        remove([leg])

def build():
    bookcase()
    small_cabinet()
    for name in ('CoffeeTable','WindowConsole'):
        rectangular_table(name)
    for name in ('SideTableCup','SideTableLamp','SideTablePlant'):
        round_table(name)
    wood_chair()
