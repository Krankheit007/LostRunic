"""Reference comparison iteration; source SurfaceRevision, output Iteration02."""
import bpy,math,json,hashlib
from pathlib import Path
from mathutils import Vector
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Iteration02')
s=bpy.context.scene
if 'SurfaceRevision' not in bpy.data.filepath:raise RuntimeError('Open SurfaceRevision baseline first')
roots={o.name:[list(r) for r in o.matrix_world] for o in s.objects if o.name.startswith('ROOT_')}
def signature(o):return hashlib.sha256(str(([list(v.co) for v in o.data.vertices],[list(f.vertices) for f in o.data.polygons],[list(r) for r in o.matrix_world])).encode()).hexdigest()
floor_before=signature(s.objects['FloorEnvelope'])
mapping=json.loads((BASE.parent/'SurfaceRevision/TextureManifest.json').read_text())
for old,item in mapping.items():
    mat=bpy.data.materials.get(old)
    if not mat or not mat.use_nodes:continue
    bs=next((n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)
    if bs is None:continue
    key=item['material'].removeprefix('M_LR_')
    wood='Wood' in key
    if wood:item['BC']='T_LR_Wood_BC.png'
    for n in mat.node_tree.nodes:
        if n.type!='TEX_IMAGE' or not n.image:continue
        kind=n.label
        if kind not in item:continue
        p=BASE/'Textures'/item[kind]
        im=bpy.data.images.load(str(p),check_existing=True)
        im.colorspace_settings.name='sRGB' if kind=='BC' else 'Non-Color'
        n.image=im
    if key in ('CoolPlaster','FloorMass') or wood:
        # Discard mismatched old normal and old seam masks; never multiply unrelated maps.
        for socket in ('Normal','Roughness'):
            for link in list(bs.inputs[socket].links):mat.node_tree.links.remove(link)
        bs.inputs['Roughness'].default_value=.82 if key=='CoolPlaster' else (.76 if key=='FloorMass' else .68)
        item['Normal']=None;item['Roughness']=None
        item['roughness']=bs.inputs['Roughness'].default_value
        item['SMK']='T_LR_Neutral_SMK.png'
        node=mat.node_tree.nodes.get('LR_SMK')
        if node:
            im=bpy.data.images.load(str(BASE/'Textures/T_LR_Neutral_SMK.png'),check_existing=True);im.colorspace_settings.name='Non-Color';node.image=im
        item['normal']='Omitted: old generated normal does not correspond to revised albedo'
    if any(t in key for t in ('Cloth','Throw','Curtain')):
        bs.inputs['Specular IOR Level'].default_value=.22
        bs.inputs['Sheen Weight'].default_value=.12
    mat['texture_contract']=json.dumps(item)

# 2m floor/wall tiling; 1m walnut tiling. Geometry stays unchanged here.
for o in s.objects:
    if o.type!='MESH':continue
    mats=' '.join(m.name for m in o.data.materials if m)
    if o.parent is None:
        for loop in o.data.uv_layers.active.data:loop.uv/=2

changed=[]
def piping(o,axis):
    points=[v.co for v in o.data.vertices]
    lo=Vector(tuple(min(v[i] for v in points) for i in range(3)))
    hi=Vector(tuple(max(v[i] for v in points) for i in range(3)))
    center=(lo+hi)/2;half=(hi-lo)/2
    axes=[i for i in range(3) if i!=axis]
    curve=bpy.data.curves.new('UpholsteryWelt','CURVE');curve.dimensions='3D'
    curve.bevel_depth=.0022;curve.bevel_resolution=1;curve.resolution_u=1
    spline=curve.splines.new('POLY');spline.points.add(63);spline.use_cyclic_u=True
    for j,point in enumerate(spline.points):
        a=j*math.tau/64;co=center.copy()
        co[axes[0]]+=half[axes[0]]*.994*math.copysign(abs(math.cos(a))**.42,math.cos(a))
        co[axes[1]]+=half[axes[1]]*.994*math.copysign(abs(math.sin(a))**.42,math.sin(a))
        point.co=(*co,1)
    new=bpy.data.objects.new('R02_Welt_'+o.name,curve);s.collection.objects.link(new)
    new.parent=o.parent;new.matrix_basis=o.matrix_basis.copy();new.data.materials.append(o.data.materials[0])
    bpy.ops.object.select_all(action='DESELECT');new.select_set(True);bpy.context.view_layer.objects.active=new
    bpy.ops.object.convert(target='MESH')
    new=bpy.context.object;new['shell']='closed';new['level']=3
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    new.data.uv_layers.active.name='UV0'

for rootname in ('ROOT_Sofa','ROOT_ArmchairOchre','ROOT_ArmchairRusset'):
    p=s.objects[rootname]
    for o in list(p.children):
        if o.type!='MESH':continue
        name=o.name
        lo=Vector(tuple(min(v.co[i] for v in o.data.vertices) for i in range(3)))
        hi=Vector(tuple(max(v.co[i] for v in o.data.vertices) for i in range(3)))
        mid=(lo+hi)/2;half=(hi-lo)/2
        mode=next((k for k in ('SeatCushion','BackCushion','LooseCushion','RolledArm','WingEnvelope','ArmSideEnvelope') if k in name),None)
        if not mode:continue
        for v in o.data.vertices:
            q=v.co-mid
            if mode=='SeatCushion':
                v.co.z=mid.z+q.z*.77
                if q.z>0:v.co.z-=.011*math.exp(-((q.x/max(half.x,.01))**2+(q.y/max(half.y,.01))**2)*3)
            elif mode in ('BackCushion','LooseCushion'):
                v.co.y=mid.y+q.y*(.72 if mode=='BackCushion' else .78)
                if mode=='BackCushion':
                    # Gentle narrowing at waist, preserving upper width.
                    v.co.x=mid.x+q.x*(.97-.05*math.exp(-((q.z/half.z+.35)/.4)**2))
                else:
                    v.co.z+=.009*math.sin(q.x/max(half.x,.01)*2.5)
            elif mode=='RolledArm':
                v.co.z=mid.z+q.z*.70
                v.co.x=mid.x+q.x*.88
            elif mode=='WingEnvelope':v.co.x=mid.x+q.x*.72
            elif mode=='ArmSideEnvelope':v.co.x=mid.x+q.x*.84
        o.data.update()
        if o.data.has_custom_normals:o.data.normals_split_custom_set([(0,0,0)]*len(o.data.loops))
        for face in o.data.polygons:face.use_smooth=True
        changed.append(o.name)
        if mode=='SeatCushion':piping(o,2)
        if mode=='BackCushion':piping(o,1)
bpy.context.view_layer.update()
assert floor_before==signature(s.objects['FloorEnvelope'])
assert all(max(abs(s.objects[n].matrix_world[i][j]-m[i][j]) for i in range(4) for j in range(4))<1e-6 for n,m in roots.items())
(BASE/'TextureManifest.json').write_text(json.dumps(mapping,indent=2))
(BASE/'Changes.json').write_text(json.dumps({'changed_geometry':changed,'layout_root_matrices':'UNCHANGED','floor_geometry':'UNCHANGED, continuous, no bevel','lighting_camera':'unchanged baseline for comparison'},indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'LivingRoom_Iteration02.blend'))
print('Iteration02 saved; reshaped',len(changed),'soft upholstery pieces')
