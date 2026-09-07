"""Rebuild Level 2 from the immutable, user-approved Level 1 .blend.

Never run generate_stage01.py here: the approved file contains user-authored
architecture and transforms which supersede that historical generator.
"""
import bpy
import sys
import json
import hashlib
import importlib
from pathlib import Path

BASE=Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
SOURCE=BASE/'Approved/LivingRoom_Level01_UserApproved.blend'
OUT=BASE/'Stage02'
OUT.mkdir(parents=True,exist_ok=True)
SCRIPTS=Path('D:/25DGame/LostRunic/Tools/Blender/LivingRoom')
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0,str(SCRIPTS))
if Path(bpy.data.filepath).resolve()!=SOURCE.resolve():
    raise RuntimeError('Open the approved baseline in a separate MCP call before building.')
scene=bpy.data.scenes['LR_LivingRoom_Stage01']
bpy.context.window.scene=scene
scene.name='LR_LivingRoom_Stage02'

def matrix_list(obj):
    return [[float(v) for v in row] for row in obj.matrix_world]

def mesh_signature(obj):
    data={'matrix':matrix_list(obj),'vertices':[list(v.co) for v in obj.data.vertices],
        'faces':[list(p.vertices) for p in obj.data.polygons]}
    return hashlib.sha256(json.dumps(data,sort_keys=True).encode()).hexdigest()

roots={o.name:matrix_list(o) for o in scene.objects if o.name.startswith('ROOT_')}
architecture={o.name:mesh_signature(o) for o in scene.objects if o.type=='MESH' and o.parent is None}
snapshot={'source':str(SOURCE),'sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
    'level_1_gate':'USER_CORRECTED_AND_AUTHORIZED_LEVEL_2',
    'root_world_matrices':roots,'architecture_geometry_hashes':architecture,
    'objects':[{'name':o.name,'parent':o.parent.name if o.parent else None,'matrix_world':matrix_list(o)}
        for o in scene.objects]}
(BASE/'Approved/accepted_scene_snapshot.json').write_text(json.dumps(snapshot,indent=2),encoding='utf-8')

for module_name in ('structure_common','structure_seating','structure_casework','structure_dressing','structure_tabletop_props'):
    module=importlib.import_module(module_name)
    importlib.reload(module)
    if hasattr(module,'build'):
        module.build()

bpy.context.view_layer.update()
errors=[]
for name,matrix in roots.items():
    obj=scene.objects.get(name)
    if obj is None or matrix_list(obj)!=matrix:
        errors.append('Accepted root changed: '+name)
for name,signature in architecture.items():
    obj=scene.objects.get(name)
    if obj is None or mesh_signature(obj)!=signature:
        errors.append('Accepted architecture changed: '+name)
if errors:
    raise RuntimeError('\n'.join(errors))
scene['stage']='LEVEL_2_STRUCTURE_AWAITING_USER_REVIEW'
scene['layout_authority']=str(SOURCE)
scene['preservation']='All '+str(len(roots))+' accepted root transforms and '+str(len(architecture))+' architecture meshes match'
for o in scene.objects:
    if o.name.startswith('ROOT_'):
        o['level_1_gate']='USER_APPROVED_CORRECTED_BLEND'

# Preserve the user's source camera data. Inspection camera is a separate copy.
source_camera=scene.camera
if source_camera:
    camera=source_camera.copy()
    camera.data=source_camera.data.copy()
    camera.name='S02_Concept'
    scene.collection.objects.link(camera)
    scene.camera=camera
scene.render.engine='CYCLES'
scene.cycles.samples=32
scene.cycles.use_denoising=True
scene.render.resolution_x=1400
scene.render.resolution_y=1100
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(OUT/'01_Concept.png')
# Restore only image visibility intentionally suppressed by our earlier review script.
for o in scene.objects:
    if o.name.startswith('ImagePlaceholder') or o.parent and o.parent.name=='ROOT_PaintingLeft':
        o.hide_render=False

(OUT/'preservation.json').write_text(json.dumps({'status':'PASS','accepted_roots':len(roots),
    'unchanged_architecture_meshes':len(architecture),'errors':errors,
    'source_sha256':snapshot['sha256']},indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LivingRoom_Stage02.blend'))
print(json.dumps({'stage':2,'preserved_roots':len(roots),'preserved_architecture':len(architecture),
    'objects':len(scene.objects),'blend':str(OUT/'LivingRoom_Stage02.blend')}))
