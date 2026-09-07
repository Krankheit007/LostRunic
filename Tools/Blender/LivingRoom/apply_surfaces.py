"""Apply generated material library without UE NPR emulation."""
import bpy,json
from pathlib import Path
BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Delivery')
mapping=json.loads((BASE/'TextureManifest.json').read_text())
for old,item in mapping.items():
    mat=bpy.data.materials.get(old)
    if not mat:continue
    mat.use_nodes=True
    shader=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    for suffix,space in [('BC','sRGB'),('SMK','Non-Color')]:
        im=bpy.data.images.load(str(BASE/'Textures'/item[suffix]),check_existing=True)
        im.colorspace_settings.name=space
        node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im
        node.label=suffix;node.name='LR_'+suffix
        if suffix=='BC':mat.node_tree.links.new(node.outputs['Color'],shader.inputs['Base Color'])
    shader.inputs['Roughness'].default_value=item['roughness']
    shader.inputs['Metallic'].default_value=item['metallic']
    mat['texture_contract']=json.dumps(item)
for o in bpy.context.scene.objects:
    if o.type!='MESH':continue
    # Large planar rug tops need intact bordered patterns, not packed polygon islands.
    if o.parent and o.parent.name.startswith('ROOT_Rug'):
        uv=o.data.uv_layers.active
        xs=[v.co.x for v in o.data.vertices];ys=[v.co.y for v in o.data.vertices]
        for poly in o.data.polygons:
            if abs(poly.normal.z)>.7:
                for i in poly.loop_indices:
                    co=o.data.vertices[o.data.loops[i].vertex_index].co
                    uv.data[i].uv=((co.x-min(xs))/(max(xs)-min(xs)),(co.y-min(ys))/(max(ys)-min(ys)))
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'LivingRoom.blend'))
print('Surface candidate saved')
