"""Eliminate planar cloth stretch through box-projection bake onto packed UV0."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Iteration02')
s=bpy.context.scene
s.render.engine='CYCLES';s.cycles.samples=8
s.render.bake.use_pass_direct=False;s.render.bake.use_pass_indirect=False;s.render.bake.use_pass_color=True
s.render.bake.margin=8

def pillow(o):
    # Thin sewn perimeter and broad fabric bulge; corners retain the reference's pointed shape.
    old=o.data;coords=[v.co for v in old.vertices]
    lo=Vector(tuple(min(v[i] for v in coords) for i in range(3)))
    hi=Vector(tuple(max(v[i] for v in coords) for i in range(3)))
    mid=(lo+hi)/2;size=hi-lo
    n=18;verts=[];faces=[];lookup={}
    for side in (-1,1):
        for j in range(n+1):
            for i in range(n+1):
                u=2*i/n-1;v=2*j/n-1
                edge=i in (0,n) or j in (0,n)
                key=(i,j,side)
                if key not in lookup:
                    bulge=(max(0,1-u*u)*max(0,1-v*v))**.6
                    # Restrained corner wrinkles, strongest beside the stitched perimeter.
                    fold=.0025*math.sin((u-v)*24)*math.exp(-((abs(u)+abs(v)-1.65)/.24)**2)
                    lookup[key]=len(verts)
                    verts.append((mid.x+u*size.x/2,mid.y+side*(.0015+size.y*.46*bulge)+fold,mid.z+v*size.z/2+.009*u*v))
        for j in range(n):
            for i in range(n):
                ids=[]
                for a,b in ((i,j),(i+1,j),(i+1,j+1),(i,j+1)):
                    ids.append(lookup[(a,b,side)])
                faces.append(tuple(ids if side<0 else reversed(ids)))
    border=[(i,0) for i in range(n)]+[(n,j) for j in range(n)]+[(i,n) for i in range(n,0,-1)]+[(0,j) for j in range(n,0,-1)]
    for index,(i,j) in enumerate(border):
        a,b=border[(index+1)%len(border)]
        faces.append((lookup[(a,b,-1)],lookup[(i,j,-1)],lookup[(i,j,1)],lookup[(a,b,1)]))
    mesh=bpy.data.meshes.new(o.name+'_Fabric');mesh.from_pydata(verts,[],faces);mesh.update()
    for mat in old.materials:mesh.materials.append(mat)
    o.data=mesh
    for p in mesh.polygons:p.use_smooth=True
    o['shell']='closed'

for o in list(s.objects):
    if o.type=='MESH' and ('LooseCushion' in o.name or 'SmallCushion' in o.name):pillow(o)

groups={}
for o in s.objects:
    if o.type!='MESH':continue
    if not any(any(t in m.name for t in ('Cloth','Throw','Curtain')) for m in o.data.materials if m):continue
    p=o
    while p.parent:p=p.parent
    groups.setdefault(p.name,[]).append(o)
report=[]
for name,objects in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012)
    bpy.ops.object.mode_set(mode='OBJECT')
    image=bpy.data.images.new('T_LR_'+name+'_Cloth_BC',width=2048,height=2048,alpha=False)
    image.filepath_raw=str(BASE/'Textures'/(image.name+'.png'));image.file_format='PNG'
    replacements={}
    for o in objects:
        for index,old in enumerate(list(o.data.materials)):
            if old not in replacements:
                m=old.copy();m.name='R02_'+name+'_'+old.name
                nodes=m.node_tree.nodes;links=m.node_tree.links
                bc=nodes.get('LR_BC')
                if bc:
                    bs=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
                    links.new(bc.outputs['Color'],bs.inputs['Base Color'])
                    bc.projection='BOX';bc.projection_blend=.3
                    coords=nodes.new('ShaderNodeTexCoord');scale=nodes.new('ShaderNodeVectorMath');scale.operation='SCALE';scale.inputs[3].default_value=1/.65
                    links.new(coords.outputs['Object'],scale.inputs[0]);links.new(scale.outputs[0],bc.inputs['Vector'])
                for stale in list(nodes):
                    if stale.name.startswith('BAKE_TARGET'):nodes.remove(stale)
                target=nodes.new('ShaderNodeTexImage');target.name='BAKE_TARGET';target.image=image
                nodes.active=target
                replacements[old]=m
            o.data.materials[index]=replacements[old]
    # Shared atlas UV islands; clear exactly once, then accumulate objects.
    for i,o in enumerate(objects):
        bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
        s.render.bake.use_clear=(i==0)
        bpy.ops.object.bake(type='DIFFUSE')
    image.save()
    for m in replacements.values():
        nodes=m.node_tree.nodes;links=m.node_tree.links
        bs=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
        target=nodes['BAKE_TARGET'];links.new(target.outputs['Color'],bs.inputs['Base Color'])
        # Surface read is carried by the fabric geometry, with restrained matte reflection.
        for slot in ('Roughness','Normal'):
            for link in list(bs.inputs[slot].links):links.remove(link)
        bs.inputs['Roughness'].default_value=.91
        m['cloth_atlas']=image.name+'.png'
    report.append({'root':name,'meshes':len(objects),'BC':image.name+'.png','resolution':2048,'UV0':'packed group atlas; baked box mapping'})
    print('Baked cloth',name,len(objects))
(BASE/'ClothAtlasManifest.json').write_text(json.dumps(report,indent=2))
s.cycles.samples=64
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'LivingRoom_Iteration02.blend'))
