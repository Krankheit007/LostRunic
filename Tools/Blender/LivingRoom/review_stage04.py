"""Stage04 normal preview and continuous floor checker inspection via MCP."""
from pathlib import Path
p=Path('D:/25DGame/LostRunic/Tools/Blender/LivingRoom/review_stage02.py')
code=p.read_text(encoding='utf-8').replace("OUT=BASE/'Stage02'","OUT=BASE/'Stage04'")
code=code.replace("OUT/'LivingRoom_Stage02.blend'","OUT/'LivingRoom_Stage04.blend'")
exec(compile(code,str(p),'exec'))

def floor_check():
    reset()
    floor=s.objects['FloorEnvelope']
    old=list(floor.data.materials)
    mat=bpy.data.materials.new('Stage04_Floor_Checker')
    mat.use_nodes=True
    nodes=mat.node_tree.nodes
    shader=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    tex=nodes.new('ShaderNodeTexChecker');tex.inputs['Scale'].default_value=4
    tex.inputs['Color1'].default_value=(.15,.19,.23,1)
    tex.inputs['Color2'].default_value=(.65,.70,.74,1)
    uv=nodes.new('ShaderNodeTexCoord')
    mat.node_tree.links.new(uv.outputs['UV'],tex.inputs['Vector'])
    mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Base Color'])
    floor.data.materials.clear();floor.data.materials.append(mat)
    for o in s.objects:
        if o.type=='MESH':o.hide_render=o!=floor
    camera('S04_FloorTop',(0,0,15),(0,0,0),[floor])
    s.render.filepath=str(OUT/'Floor_Checker.png')
    bpy.ops.render.render(write_still=True)
    floor.data.materials.clear()
    for m in old:floor.data.materials.append(m)
    reset()
