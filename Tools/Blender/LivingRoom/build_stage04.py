"""Continuous unbeveled floor and baked surface geometry/UV preparation via MCP."""
import bpy,bmesh,json,math
from pathlib import Path
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
OUT=BASE/'Stage04'

def build():
    if Path(bpy.data.filepath).resolve()!=(BASE/'Stage03/LivingRoom_Stage03.blend').resolve():
        raise RuntimeError('Open approved Stage03 first')
    OUT.mkdir(exist_ok=True)
    s=bpy.context.scene
    roots={o.name:[list(r) for r in o.matrix_world] for o in s.objects if o.name.startswith('ROOT_')}
    floors=[s.objects[n] for n in ('FloorEnvelope','FloorEnvelopeUnderWindow','FloorEnvelopeLeftCorner')]
    source_bounds={o.name:[list(o.matrix_world@__import__('mathutils').Vector(v)) for v in o.bound_box] for o in floors}
    for o in floors:
        o.modifiers.clear()
    floor=floors[0]
    bpy.ops.object.select_all(action='DESELECT')
    floor.select_set(True);bpy.context.view_layer.objects.active=floor
    for other in floors[1:]:
        mod=floor.modifiers.new('ContinuousFloorUnion','BOOLEAN')
        mod.operation='UNION';mod.solver='EXACT';mod.object=other
        bpy.ops.object.modifier_apply(modifier=mod.name)
        bpy.data.objects.remove(other,do_unlink=True)
    bm=bmesh.new();bm.from_mesh(floor.data)
    bmesh.ops.dissolve_limit(bm,angle_limit=.001,verts=list(bm.verts),edges=list(bm.edges))
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(floor.data);bm.free()
    floor['no_bevel']=True
    floor['source_parts']='FloorEnvelope + FloorEnvelopeUnderWindow + FloorEnvelopeLeftCorner'
    floor['shell']='closed'
    for f in floor.data.polygons:f.use_smooth=False
    # Freeze the accepted silhouette radii and weighted normals into export geometry.
    meshes=[o for o in s.objects if o.type=='MESH']
    applied=0
    for o in meshes:
        bpy.context.view_layer.objects.active=o
        for mod in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=mod.name)
            applied+=1
    # Independent prop and furniture export groups receive independent UV0 packing.
    groups={}
    for o in meshes:
        p=o
        while p.parent and not p.get('independent_asset'):
            p=p.parent
        groups.setdefault(p.name,[]).append(o)
    for name,objects in groups.items():
        if objects==[floor]:
            uv=floor.data.uv_layers.active or floor.data.uv_layers.new(name='UV0')
            uv.name='UV0'
            for poly in floor.data.polygons:
                axis=max(range(3),key=lambda k:abs(poly.normal[k]))
                axes=[k for k in range(3) if k!=axis]
                for i in poly.loop_indices:
                    co=floor.matrix_world@floor.data.vertices[floor.data.loops[i].vertex_index].co
                    uv.data[i].uv=(co[axes[0]],co[axes[1]])
            floor['uv_contract']='Metric planar UV0, 1 UV unit per metre; continuous across former three slabs'
            continue
        bpy.ops.object.select_all(action='DESELECT')
        for o in objects:o.select_set(True)
        bpy.context.view_layer.objects.active=objects[0]
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012,correct_aspect=True)
        bpy.ops.object.mode_set(mode='OBJECT')
        for o in objects:
            o.data.uv_layers.active.name='UV0'
            o['uv_group']=name
    errors=[]
    for name,matrix in roots.items():
        if max(abs(s.objects[name].matrix_world[i][j]-matrix[i][j]) for i in range(4) for j in range(4))>1e-6:
            errors.append(name+': root changed')
    bm=bmesh.new();bm.from_mesh(floor.data)
    nonmanifold=sum(not e.is_manifold for e in bm.edges)
    unseen=set(bm.verts);components=0
    while unseen:
        stack=[unseen.pop()];components+=1
        while stack:
            for edge in stack.pop().link_edges:
                for v in edge.verts:
                    if v in unseen:unseen.remove(v);stack.append(v)
    volume=bm.calc_volume(signed=True);bm.free()
    if nonmanifold or components!=1 or volume<=0:errors.append('Floor topology failed')
    if floor.modifiers:errors.append('Floor must have no modifiers')
    for o in meshes:
        if not o.data.uv_layers.active:errors.append(o.name+': missing UV')
        elif any(not math.isfinite(x) for v in o.data.uv_layers.active.data for x in v.uv):errors.append(o.name+': invalid UV')
    report={'technical_status':'PASS' if not errors else 'FAIL','errors':errors,
      'floor':{'components':components,'nonmanifold_edges':nonmanifold,'signed_volume_m3':volume,'bevel':False,'source_bounds':source_bounds},
      'preserved_root_count':len(roots),'baked_modifiers':applied,'uv_groups':len(groups),
      'mesh_count':len(meshes),'triangles':sum(sum(len(f.vertices)-2 for f in o.data.polygons) for o in meshes),
      'level_3':'USER_APPROVED','uv_status':'Generated; finite UV check passed, checker visual review pending',
      'remaining':'Textures, Surface validation, per-asset FBX and roundtrip, UE actual camera'}
    (OUT/'Validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    bpy.ops.object.select_all(action='DESELECT')
    floor.select_set(True);bpy.context.view_layer.objects.active=floor
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LivingRoom_Stage04.blend'))
    print(json.dumps({k:v for k,v in report.items() if k!='floor'}))
    print('Floor:',components,'connected component;',nonmanifold,'nonmanifold edges; no bevel')

build()
