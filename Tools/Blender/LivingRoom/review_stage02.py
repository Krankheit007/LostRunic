"""Multi-view structure review and preservation checks. Execute through MCP."""
import bpy
import bmesh
import json
import math
import hashlib
from pathlib import Path
from mathutils import Vector, Matrix

BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
OUT=BASE/'Stage02'
s=bpy.data.scenes['LR_LivingRoom_Stage02']
bpy.context.window.scene=s
visibility={o.name:o.hide_render for o in s.objects}
poses={o.name:o.matrix_basis.copy() for o in s.objects if o.name.startswith('PIVOT_')}
normal_world=s.world

def descendants(parent):
    result=list(parent.children_recursive)
    return [o for o in result if o.type=='MESH']

def reset():
    for o in s.objects:
        o.hide_render=visibility.get(o.name,False)
    for name,pose in poses.items():
        s.objects[name].matrix_basis=pose
    s.camera=s.objects['S02_Concept']
    s.view_layers[0].material_override=None
    s.world=normal_world
    s.render.resolution_x,s.render.resolution_y=1400,1100

def camera(name,offset,target,meshes):
    obj=s.objects.get(name)
    if not obj:
        data=bpy.data.cameras.new(name)
        data.type='ORTHO'
        obj=bpy.data.objects.new(name,data)
        s.collection.objects.link(obj)
    obj.location=Vector(target)+Vector(offset)
    obj.rotation_euler=(-Vector(offset)).to_track_quat('-Z','Y').to_euler()
    rot=obj.rotation_euler.to_matrix()
    right,up=rot@Vector((1,0,0)),rot@Vector((0,1,0))
    corners=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    xs=[v.dot(right) for v in corners];ys=[v.dot(up) for v in corners]
    target=Vector(target)
    obj.location+=right*((max(xs)+min(xs))/2-target.dot(right))
    obj.location+=up*((max(ys)+min(ys))/2-target.dot(up))
    aspect=s.render.resolution_x/s.render.resolution_y
    obj.data.ortho_scale=max(max(xs)-min(xs),(max(ys)-min(ys))*aspect)*1.10
    s.camera=obj
    return obj

def render_scene(name):
    reset()
    if name=='Reverse':
        for o in s.objects:
            if o.type=='MESH' and o.parent is None and 'Floor' not in o.name:
                o.hide_render=True
            p=o
            while p.parent:
                p=p.parent
            if p.name in ('ROOT_Window','ROOT_Curtains','ROOT_BackDoor') or p.get('category')=='WallDecor':
                o.hide_render=True
    offsets={'Concept':(-9,-12,11.05),'Reverse':(9,11,9),'Top':(0,0,15)}
    objs=[o for o in s.objects if o.type=='MESH' and not o.hide_render]
    camera('S02_Review_'+name,offsets[name],(0,0,1),objs)
    s.render.filepath=str(OUT/('01_Concept.png' if name=='Concept' else '02_'+name+'.png'))
    bpy.ops.render.render(write_still=True)
    reset()
    print('Rendered '+name)

def render_asset(name,view='Front',opened=False):
    reset()
    p=s.objects['ROOT_'+name]
    if opened:
        for obj in p.children_recursive:
            if obj.name.startswith('PIVOT_'):
                if obj.get('motion_kind')=='HINGE':
                    obj.rotation_euler.z=math.radians(float(obj['suggested_limit'])*.80)
                else:
                    obj.location.y+=float(obj['suggested_limit'])
    bpy.context.view_layer.update()
    objs=descendants(p)
    for o in s.objects:
        if o.type=='MESH':
            is_ground=o.parent is None and 'Floor' in o.name
            o.hide_render=o not in objs and not is_ground
    s.render.resolution_x,s.render.resolution_y=1100,900
    offset=Vector((3,5,2.5)) if view=='Front' else Vector((-3,-5,2.5))
    offset=p.matrix_world.to_quaternion()@offset
    camera('S02_Asset_'+name+'_'+view,offset,p.matrix_world.translation,objs)
    s.render.filepath=str(OUT/(name+'_'+view+('_Open' if opened else '')+'.png'))
    bpy.ops.render.render(write_still=True)
    reset()
    print('Rendered '+name+' '+view)

def render_detail(name,z_min):
    reset()
    p=s.objects['ROOT_'+name]
    bpy.context.view_layer.update()
    inv=p.matrix_world.inverted()
    objs=[o for o in descendants(p) if max((inv@o.matrix_world@Vector(c)).z for c in o.bound_box)>z_min]
    for o in s.objects:
        if o.type=='MESH':
            o.hide_render=o not in objs
    s.render.resolution_x,s.render.resolution_y=1100,850
    offset=p.matrix_world.to_quaternion()@Vector((2,3.5,2.8))
    camera('S02_Detail_'+name,offset,p.matrix_world.translation,objs)
    s.render.filepath=str(OUT/('Detail_'+name+'.png'))
    bpy.ops.render.render(write_still=True)
    reset()
    print('Rendered detail '+name)

def validate():
    reset()
    bpy.context.view_layer.update()
    baseline=json.loads((BASE/'Approved/accepted_scene_snapshot.json').read_text(encoding='utf-8'))
    errors=[]
    for name,rows in baseline['root_world_matrices'].items():
        obj=s.objects.get(name)
        if not obj or max(abs(obj.matrix_world[i][j]-rows[i][j]) for i in range(4) for j in range(4))>1e-6:
            errors.append({'object':name,'issue':'accepted root changed'})
    for name,signature in baseline['architecture_geometry_hashes'].items():
        obj=s.objects[name]
        data={'matrix':[[float(v) for v in row] for row in obj.matrix_world],
            'vertices':[list(v.co) for v in obj.data.vertices],
            'faces':[list(p.vertices) for p in obj.data.polygons]}
        actual=hashlib.sha256(json.dumps(data,sort_keys=True).encode()).hexdigest()
        if actual!=signature:
            errors.append({'object':name,'issue':'accepted architecture changed'})
    open_surfaces=[]
    for obj in s.objects:
        if obj.type!='MESH' or obj.get('level')!=2:
            continue
        bm=bmesh.new();bm.from_mesh(obj.data)
        if obj.get('shell','').startswith('open'):
            open_surfaces.append(obj.name)
        elif any(not e.is_manifold for e in bm.edges):
            errors.append({'object':obj.name,'issue':'nonmanifold declared closed primitive'})
        if any(f.calc_area()<1e-10 for f in bm.faces):
            errors.append({'object':obj.name,'issue':'degenerate face'})
        bm.free()
    dg=bpy.context.evaluated_depsgraph_get()
    assets=[]
    for p in s.objects:
        if not p.name.startswith('ROOT_'):
            continue
        objs=descendants(p)
        tris=0
        for o in objs:
            e=o.evaluated_get(dg);m=e.to_mesh();m.calc_loop_triangles()
            tris+=len(m.loop_triangles);e.to_mesh_clear()
        corners=[o.matrix_world@Vector(c) for o in objs for c in o.bound_box]
        assets.append({'id':p.name,'mesh_count':len(objs),'evaluated_triangles':tris,
            'world_matrix':[[float(v) for v in row] for row in p.matrix_world],
            'bounds_min_m':[min(v[i] for v in corners) for i in range(3)],
            'bounds_max_m':[max(v[i] for v in corners) for i in range(3)],
            'structure':p.get('structure','retained / dressing'),
            'pivots':[{'name':o.name,'kind':o.get('motion_kind'),'axis':o.get('local_axis'),
                'limit':o.get('suggested_limit'),'state':o.get('preview_state')}
                for o in p.children_recursive if o.name.startswith('PIVOT_')]})
    props=[{'id':p.name,'support':p.parent.name,'description':p.get('source_visible_structure'),
        'world_matrix':[[float(v) for v in row] for row in p.matrix_world],
        'mesh_count':len(descendants(p)),'export_contract':p.get('export_contract')}
        for p in s.objects if p.get('independent_asset')]
    (OUT/'Manifest.json').write_text(json.dumps({'level':2,'units':'metres',
        'layout_authority':baseline['source'],'assets':assets,'independent_props':props,
        'final_fbx':'not exported before gates'},indent=2),encoding='utf-8')
    report={'technical_status':'PASS' if not errors else 'FAIL','errors':errors,
        'accepted_root_count':len(baseline['root_world_matrices']),
        'unchanged_architecture_count':len(baseline['architecture_geometry_hashes']),
        'declared_open_surfaces':open_surfaces,'level_1':'USER_APPROVED_CORRECTED_BLEND',
        'level_2':'AWAITING_USER_STRUCTURE_REVIEW',
        'mesh_count':sum(a['mesh_count'] for a in assets),
        'triangles':sum(a['evaluated_triangles'] for a in assets),'independent_prop_count':len(props),
        'fbx_uv_normals_surface_animation':'final validation deferred to later gates'}
    (OUT/'Validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='declared_open_surfaces'}))

def save():
    reset()
    if s.objects.get('S02_Review_Concept'):
        s.camera=s.objects['S02_Review_Concept']
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LivingRoom_Stage02.blend'))
