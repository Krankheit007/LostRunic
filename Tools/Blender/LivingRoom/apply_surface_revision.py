"""Replace rejected pastel surfaces; preserve mesh geometry and root placement."""
import bpy,json,math,hashlib
from pathlib import Path
from mathutils import Vector
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/SurfaceRevision')
s=bpy.context.scene
def geometry_signature():
    return {o.name:hashlib.sha256(str(([list(v.co) for v in o.data.vertices],[list(f.vertices) for f in o.data.polygons],[list(r) for r in o.matrix_world])).encode()).hexdigest() for o in s.objects if o.type=='MESH'}
before=geometry_signature()
mapping=json.loads((BASE/'TextureManifest.json').read_text())
for old,item in mapping.items():
    mat=bpy.data.materials.get(old)
    if not mat:continue
    mat.use_nodes=True;nodes=mat.node_tree.nodes;links=mat.node_tree.links
    nodes.clear();out=nodes.new('ShaderNodeOutputMaterial');bs=nodes.new('ShaderNodeBsdfPrincipled')
    links.new(bs.outputs['BSDF'],out.inputs['Surface'])
    for kind,space in [('BC','sRGB'),('SMK','Non-Color'),('Roughness','Non-Color'),('Normal','Non-Color')]:
        im=bpy.data.images.load(str(BASE/'Textures'/item[kind]),check_existing=True);im.colorspace_settings.name=space
        n=nodes.new('ShaderNodeTexImage');n.image=im;n.label=kind;n.name='LR_'+kind
        if kind=='BC':links.new(n.outputs['Color'],bs.inputs['Base Color'])
        if kind=='Roughness':links.new(n.outputs['Color'],bs.inputs['Roughness'])
        if kind=='Normal':
            sep=nodes.new('ShaderNodeSeparateColor');combine=nodes.new('ShaderNodeCombineColor')
            inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
            links.new(n.outputs['Color'],sep.inputs['Color']);links.new(sep.outputs['Green'],inv.inputs[1])
            links.new(sep.outputs['Red'],combine.inputs['Red']);links.new(inv.outputs[0],combine.inputs['Green']);links.new(sep.outputs['Blue'],combine.inputs['Blue'])
            norm=nodes.new('ShaderNodeNormalMap');norm.inputs['Strength'].default_value=.4
            links.new(combine.outputs['Color'],norm.inputs['Color']);links.new(norm.outputs['Normal'],bs.inputs['Normal'])
    bs.inputs['Metallic'].default_value=item['metallic']
    mat['texture_contract']=json.dumps(item)
    mat['surface_revision']='Aged reference palette; replaces rejected pastel material'

# Physical projection keeps architectural grain scale independent of island packing.
for o in s.objects:
    if o.type!='MESH':continue
    mats=' '.join(m.name for m in o.data.materials if m)
    cloth=any(t in mats for t in ('Cloth','TerracottaThrow','CurtainMass'))
    if o.name.startswith('FloorEnvelope') or (o.parent is None) or 'Wood' in mats or cloth:
        uv=o.data.uv_layers.active
        for poly in o.data.polygons:
            axis=max(range(3),key=lambda k:abs(poly.normal[k]))
            if cloth:axis=min(range(3),key=lambda k:o.dimensions[k])
            axes=[i for i in range(3) if i!=axis]
            for i in poly.loop_indices:
                co=o.data.vertices[o.data.loops[i].vertex_index].co
                if o.parent is None:co=o.matrix_world@co
                if cloth:co=co/.65
                uv.data[i].uv=(co[axes[0]],co[axes[1]])

# Reuse the actual visible painting interiors from the user's source; no invented art.
quads=[[(441,108),(545,65),(548,225),(445,268)],
       [(758,60),(782,51),(780,113),(758,124)],
       [(844,63),(883,82),(883,146),(844,126)],
       [(1355,335),(1404,369),(1397,472),(1350,433)],
       [(64,658),(123,600),(143,789),(91,819)]]
im=bpy.data.images.load(str(BASE/'Textures/ConceptArtworkSource.png'),check_existing=True)
for j,o in enumerate(sorted([o for o in s.objects if o.name.startswith('ImagePlaceholder')],key=lambda o:o.name)):
    mat=bpy.data.materials.new('M_LR_ReferencePainting_'+str(j));mat.use_nodes=True
    bs=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    n=mat.node_tree.nodes.new('ShaderNodeTexImage');n.image=im
    mat.node_tree.links.new(n.outputs['Color'],bs.inputs['Base Color']);bs.inputs['Roughness'].default_value=.9
    o.data.materials.clear();o.data.materials.append(mat)
    uv=o.data.uv_layers.active
    xs=[v.co.x for v in o.data.vertices];zs=[v.co.z for v in o.data.vertices]
    tl,tr,br,bl=[Vector((a/1448,1-b/1086)) for a,b in quads[j]]
    for poly in o.data.polygons:
        for i in poly.loop_indices:
            co=o.data.vertices[o.data.loops[i].vertex_index].co
            u=(co.x-min(xs))/(max(xs)-min(xs));v=(co.z-min(zs))/(max(zs)-min(zs))
            uv.data[i].uv=bl*(1-u)*(1-v)+br*u*(1-v)+tl*(1-u)*v+tr*u*v

for o in s.objects:
    if o.type=='LIGHT':o.hide_render=True
bg=next(n for n in s.world.node_tree.nodes if n.type=='BACKGROUND')
bg.inputs['Color'].default_value=(.32,.42,.60,1);bg.inputs['Strength'].default_value=.22
def light(name,kind,loc,color,energy,target=None,size=.3):
    data=bpy.data.lights.new(name,kind);data.energy=energy;data.color=color
    if kind=='AREA':data.shape='DISK';data.size=size
    if kind=='POINT':data.shadow_soft_size=size
    o=bpy.data.objects.new(name,data);s.collection.objects.link(o);o.location=loc
    if target:o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
    return o
light('Reference_WindowSoft','AREA',(3.35,.5,2.8),(1,.83,.61),500,(0,-.5,.4),2.0)
light('Reference_CoolBounce','AREA',(-3,-4,5),(.58,.70,1),230,(0,0,.7),5.0)
sun=light('Reference_AfternoonSun','SUN',(6,3,7),(1,.77,.48),1.5,(-3,-2,0));sun.data.angle=.04
for o in list(s.objects):
    if not o.name.startswith('ROOT_') or 'Lamp' not in o.name or 'SideTable' in o.name:continue
    meshes=[c for c in o.children_recursive if c.type=='MESH']
    if not meshes:continue
    points=[c.matrix_world@Vector(b) for c in meshes for b in c.bound_box]
    center=o.matrix_world.translation.copy();center.z=max(v.z for v in points)-.13
    if 'Wall' in o.name:center.y-=.14
    light('Reference_'+o.name,'POINT',center,(1,.56,.20),22,None,.07)
fire=s.objects['ROOT_Fireplace'].matrix_world@Vector((0,.27,.48))
light('Reference_FireGlow','POINT',fire,(1,.29,.055),18,None,.18)
for mat in bpy.data.materials:
    if mat.name.startswith('S01_WindowPlaceholder') and mat.use_nodes:
        mat.node_tree.nodes.clear();out=mat.node_tree.nodes.new('ShaderNodeOutputMaterial')
        glass=mat.node_tree.nodes.new('ShaderNodeBsdfTransparent');glass.inputs[0].default_value=(.84,.89,.92,1)
        mat.node_tree.links.new(glass.outputs[0],out.inputs['Surface'])
s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast';s.view_settings.exposure=.0
s.cycles.samples=64
assert before==geometry_signature(),'Geometry or layout changed unexpectedly'
(BASE/'Preservation.json').write_text(json.dumps({'geometry_and_world_transforms':'UNCHANGED','mesh_count':len(before),'floor':'unchanged continuous no-bevel mesh'},indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'LivingRoom_MaterialRevision.blend'))
print('Materials, artwork and reference preview lighting revised; geometry preserved')
