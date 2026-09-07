"""Independent binary FBX assets at local origins; roundtrip every output."""
import bpy,json,re,math,bmesh
from pathlib import Path
from mathutils import Vector,Matrix
OUT=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Delivery')
FBX=OUT/'FBX';FBX.mkdir(exist_ok=True)
SOURCE=bpy.context.scene

def owner(o):
    p=o
    while p.parent and not p.get('independent_asset'):p=p.parent
    return p

def bounds(objects):
    bpy.context.view_layer.update()
    co=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
    return [[min(v[i] for v in co) for i in range(3)],[max(v[i] for v in co) for i in range(3)]]

def stats(objects):
    return {'mesh_count':len(objects),'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objects),
      'bounds':bounds(objects),'materials':sorted({m.name for o in objects for m in o.data.materials if m}),
      'uv_missing':[o.name for o in objects if not o.data.uv_layers.active]}

def export_all():
    groups={}
    for o in SOURCE.objects:
        if o.type=='MESH':groups.setdefault(owner(o),[]).append(o)
    records=[]
    for index,(p,meshes) in enumerate(groups.items()):
        name='SM_LR_'+re.sub(r'[^A-Za-z0-9_]','_',p.name.removeprefix('ROOT_').removeprefix('PROP_'))
        temp=bpy.data.scenes.new('FBX_EXPORT_TEMP');bpy.context.window.scene=temp
        temp.unit_settings.system='METRIC';temp.unit_settings.scale_length=1
        copies={}
        required=set(meshes)
        for o in meshes:
            a=o.parent
            while a and a!=p:required.add(a);a=a.parent
        inv=p.matrix_world.inverted()
        root=bpy.data.objects.new('ROOT_'+name,None);temp.collection.objects.link(root)
        for o in required:
            c=o.copy()
            if o.type=='MESH':c.data=o.data.copy()
            temp.collection.objects.link(c);copies[o]=c
        for o,c in copies.items():
            c.parent=copies.get(o.parent,root)
            c['export_id']=o.name
        def depth(o):
            n=0
            while o.parent:n+=1;o=o.parent
            return n
        for o in sorted(copies,key=depth):
            c=copies[o]
            c.matrix_world=inv@o.matrix_world
            bpy.context.view_layer.update()
            if c.type=='MESH':
                bpy.context.view_layer.objects.active=c
                mod=c.modifiers.new('ExportTriangulation','TRIANGULATE')
                mod.keep_custom_normals=True
                bpy.ops.object.modifier_apply(modifier=mod.name)
        outmeshes=[c for c in copies.values() if c.type=='MESH']
        before=stats(outmeshes)
        expected={o.name:{'matrix':[list(r) for r in inv@o.matrix_world],
           'parent':o.parent.name if o.parent in copies else None,
           'shell':o.get('shell','unspecified')} for o in required}
        path=FBX/(name+'.fbx')
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=False,object_types={'EMPTY','MESH'},
          axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,
          apply_scale_options='FBX_SCALE_UNITS',use_mesh_modifiers=True,mesh_smooth_type='FACE',
          bake_anim=False,add_leaf_bones=False,path_mode='RELATIVE',embed_textures=False,use_custom_props=True)
        binary=path.read_bytes()[:20].startswith(b'Kaydara FBX Binary')
        bpy.context.window.scene=SOURCE
        for o in list(temp.objects):bpy.data.objects.remove(o,do_unlink=True)
        bpy.data.scenes.remove(temp)
        check=bpy.data.scenes.new('FBX_ROUNDTRIP_TEMP');bpy.context.window.scene=check
        bpy.ops.import_scene.fbx(filepath=str(path),use_anim=False)
        imported=[o for o in check.objects if o.type=='MESH']
        after=stats(imported)
        # Import adds numeric material suffixes when the source material is already loaded.
        normalize=lambda ns:sorted({re.sub(r'(\.\d+)+$','',n) for n in ns})
        delta=max(abs(before['bounds'][j][i]-after['bounds'][j][i]) for j in range(2) for i in range(3))
        errors=[]
        if not binary:errors.append('not binary FBX')
        if before['mesh_count']!=after['mesh_count']:errors.append('mesh count changed')
        if before['triangles']!=after['triangles']:errors.append('triangle count changed')
        if delta>.0001:errors.append('bounds/scale changed')
        if normalize(before['materials'])!=normalize(after['materials']):errors.append('material slots changed')
        if after['uv_missing']:errors.append('UV missing')
        if any(not all(math.isfinite(x) for x in v.normal) for o in imported for v in o.data.vertices):errors.append('invalid normal')
        shells=[]
        for o in check.objects:
            identity=o.get('export_id')
            if identity not in expected:continue
            contract=expected[identity]
            matrix_delta=max(abs(o.matrix_world[i][j]-contract['matrix'][i][j]) for i in range(4) for j in range(4))
            if matrix_delta>.0001:errors.append(identity+': local transform changed')
            parent_id=o.parent.get('export_id') if o.parent else None
            if parent_id!=contract['parent']:errors.append(identity+': parent changed')
            if o.type=='MESH' and contract['shell']=='closed':
                bm=bmesh.new();bm.from_mesh(o.data)
                count=sum(not e.is_manifold for e in bm.edges);bm.free()
                shells.append({'id':identity,'nonmanifold_edges':count})
                if count:errors.append(identity+': declared closed shell not manifold')
        records.append({'id':name,'file':'FBX/'+path.name,'source':p.name,
          'assembly_world_matrix':[list(r) for r in p.matrix_world],
          'pivot':'original asset local origin; static; no animation Actions baked',
          'before':before,'roundtrip':after,'closed_shell_checks':shells,'max_bounds_delta_m':delta,'binary':binary,'errors':errors})
        bpy.context.window.scene=SOURCE
        for o in list(check.objects):bpy.data.objects.remove(o,do_unlink=True)
        bpy.data.scenes.remove(check)
        (OUT/'Manifest.json').write_text(json.dumps({'units':'metres; FBX unit metadata converts to UE centimetres','axis':'-Y forward / Z up','assets':records},indent=2),encoding='utf-8')
        print(index+1,'/',len(groups),name,'PASS' if not errors else errors)
    (OUT/'Validation.json').write_text(json.dumps({'automatic_status':'PASS' if all(not r['errors'] for r in records) else 'FAIL',
      'fbx_count':len(records),'visual_acceptance':'AWAITING_USER_SURFACE_REVIEW','UE_import_gameplay':'NOT_TESTED',
      'animation':'Static FBX; existing pivot hierarchy retained, no Actions requested or baked',
      'floor':'single continuous closed floor, no bevel; metric UV',
      'limitations':['Texture direction/UV stretch requires final visual acceptance','No collision meshes or lightmap UV generated','Closed-shell audit applies only to meshes declared closed; open cloth and foliage are explicit exceptions']},indent=2),encoding='utf-8')
    print('All FBXs exported and roundtripped')

export_all()
