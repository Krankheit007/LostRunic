"""Level 2 modelling helpers; no layout or architecture authoring here."""
import bpy
import math
from mathutils import Vector

def material(name, color):
    m = bpy.data.materials.get('S02_'+name)
    if m:
        return m
    m = bpy.data.materials.new('S02_'+name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    shader = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = (*color, 1)
    shader.inputs['Roughness'].default_value = .82
    return m

WOOD = material('Wood', (.17,.105,.065))
WOOD_LIGHT = material('WoodPanel', (.205,.133,.078))
DARK = material('Interior', (.065,.052,.04))
CREAM = material('CreamCloth', (.69,.63,.50))
THROW = material('TerracottaThrow', (.34,.15,.12))
LEAF = material('Leaf', (.15,.20,.095))
LEAF_LIGHT = material('LeafLight', (.22,.28,.12))
IRON = material('Iron', (.055,.052,.047))

def root(name):
    return bpy.context.scene.objects['ROOT_'+name]

def children(parent, prefix):
    return [o for o in parent.children if o.name.startswith(prefix)]

def remove(objects):
    for obj in list(objects):
        bpy.data.objects.remove(obj, do_unlink=True)

def attach(obj, parent, name, mat):
    obj.name = 'S02_'+parent.name.removeprefix('ROOT_')+'_'+name
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    parent.users_collection[0].objects.link(obj)
    obj.parent = parent
    obj.data.materials.append(mat)
    obj['level'] = 2
    obj['shell'] = 'closed'
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj

def rounding(obj, radius):
    if radius:
        mod = obj.modifiers.new('StructureRadius', 'BEVEL')
        mod.width, mod.segments = radius, 3
    normal = obj.modifiers.new('PreviewBroadNormals', 'WEIGHTED_NORMAL')
    normal.keep_sharp = True
    return obj

def box(parent, name, loc, size, mat=WOOD, radius=.006):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    attach(obj, parent, name, mat)
    return rounding(obj, radius)

def mesh(parent, name, vertices, faces, mat=WOOD, smooth=True):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    attach(obj, parent, name, mat)
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    for p in data.polygons:
        p.use_smooth = smooth
    return obj

def loft(parent, name, rings, mat=WOOD, sides=12):
    # rings: x,y,z,rx,ry. Ring orientation is horizontal.
    vertices = [(x+rx*math.cos(2*math.pi*j/sides), y+ry*math.sin(2*math.pi*j/sides), z)
        for x,y,z,rx,ry in rings for j in range(sides)]
    faces = [tuple(reversed(range(sides)))]
    for k in range(len(rings)-1):
        for j in range(sides):
            a=k*sides+j; b=k*sides+(j+1)%sides
            faces.append((a,b,b+sides,a+sides))
    faces.append(tuple((len(rings)-1)*sides+j for j in range(sides)))
    return mesh(parent,name,vertices,faces,mat)

def beam(parent,name,a,b,r,mat=WOOD):
    a,b=Vector(a),Vector(b)
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=r, depth=(b-a).length,
        location=(a+b)/2)
    obj = bpy.context.object
    obj.rotation_euler = (b-a).to_track_quat('Z','Y').to_euler()
    return rounding(attach(obj,parent,name,mat),.002)

def puff(parent,name,loc,size,mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,location=loc)
    obj=bpy.context.object
    for v in obj.data.vertices:
        for i in range(3):
            n=v.co[i]
            v.co[i]=math.copysign(abs(n)**.42,n)*size[i]/2
    return attach(obj,parent,name,mat)

def shaped_leg(parent,loc,height,radius=.032,flared=.016):
    x,y=loc
    sx=1 if x>0 else -1
    sy=1 if y>0 else -1
    return loft(parent,'Leg',[
        (x+sx*flared,y+sy*flared,0,radius*.60,radius*.68),
        (x+sx*flared*.7,y+sy*flared*.7,height*.12,radius*.66,radius*.70),
        (x-sx*flared*.4,y-sy*flared*.4,height*.42,radius*.64,radius*.64),
        (x+sx*flared*.25,y+sy*flared*.25,height*.76,radius*1.12,radius),
        (x,y,height,radius,radius)],sides=12)

def pivot(parent,name,loc,kind,axis,limit):
    obj=bpy.data.objects.new('PIVOT_'+parent.name.removeprefix('ROOT_')+'_'+name,None)
    parent.users_collection[0].objects.link(obj)
    obj.parent=parent
    obj.location=loc
    obj['motion_kind']=kind
    obj['local_axis']=axis
    obj['suggested_limit']=limit
    obj['preview_state']='Closed; no keyframes. Final runtime animation is not implemented.'
    return obj

def reparent_keep_local(obj,new_parent):
    # Both transforms are expressed relative to the same asset root.
    matrix=obj.matrix_basis.copy()
    obj.parent=new_parent
    obj.matrix_basis=new_parent.matrix_basis.inverted()@matrix

def drawer(parent,name,x,z,width,depth,height,front_y):
    p=pivot(parent,name,(x,front_y,z),'SLIDE','+Y',round(depth*.65,3))
    bottom=box(parent,name+'_Bottom',(x,front_y-depth/2,z-height/2+.012),
        (width-.025,depth,.024),DARK)
    pieces=[bottom,box(parent,name+'_Front',(x,front_y,z),(width,.028,height),WOOD_LIGHT)]
    for side in (-1,1):
        pieces.append(box(parent,name+'_Side',(x+side*(width/2-.016),front_y-depth/2,z),
            (.022,depth,height-.02),WOOD))
    pieces.append(box(parent,name+'_Back',(x,front_y-depth,z),(width,.022,height-.02),WOOD))
    for obj in pieces:
        reparent_keep_local(obj,p)
    return p
