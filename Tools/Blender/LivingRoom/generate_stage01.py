"""Living-room silhouette gate. Run inside Blender through Blender MCP.

Only livingroom.png supplies shapes. No final furniture or texture claim.
Scene is isolated; existing user scenes are preserved. Units are metres.
"""
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path('D:/25DGame/LostRunic/ArtSource/LivingRoom')
if (ROOT/'Approved/LivingRoom_Level01_UserApproved.blend').exists():
    raise RuntimeError('Historical Level 1 generator disabled: use the user-approved .blend and build_stage02.py.')
OUT = ROOT / 'Stage01'
OUT.mkdir(parents=True, exist_ok=True)
SCENE_NAME = 'LR_LivingRoom_Stage01'
old = bpy.data.scenes.get(SCENE_NAME)
if old:
    for obj in list(old.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.scenes.remove(old)
scene = bpy.data.scenes.new(SCENE_NAME)
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
scene['stage'] = '01_PRIMARY_SILHOUETTE_AWAITING_USER_REVIEW'
scene['source'] = 'D:/GameDesign/DontForgetAdele/livingroom.png'
scene['unseen_geometry'] = 'Backs, undersides and hidden connections inferred; not reference-verified.'

def mat(name, color):
    m = bpy.data.materials.new('S01_' + name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    bsdf = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Roughness'].default_value = .83
    return m

M = {
    'wood': mat('Wood', (.17, .105, .065)),
    'green': mat('OliveCloth', (.225, .235, .13)),
    'gold': mat('OchreCloth', (.57, .33, .075)),
    'rust': mat('RussetCloth', (.31, .135, .075)),
    'wall': mat('CoolPlaster', (.39, .42, .44)),
    'stone': mat('WarmStone', (.60, .55, .45)),
    'floor': mat('FloorMass', (.28, .245, .205)),
    'rug': mat('RugMass', (.59, .47, .33)),
    'dark': mat('DarkRecess', (.08, .075, .061)),
    'curtain': mat('CurtainMass', (.7, .65, .52)),
    'leaf': mat('FoliageMass', (.14, .205, .105)),
    'pot': mat('PotMass', (.28, .24, .17)),
    'shade': mat('LampMass', (.83, .64, .34)),
    'glass': mat('WindowPlaceholder', (.61, .72, .73)),
}
assets = []
envelope = bpy.data.collections.new('S01_ArchitectureEnvelope')
scene.collection.children.link(envelope)
current = None
root = None

def asset(name, xy=(0, 0), angle=0, category='Furniture'):
    global current, root
    current = bpy.data.collections.new('S01_' + name)
    scene.collection.children.link(current)
    root = bpy.data.objects.new('ROOT_' + name, None)
    current.objects.link(root)
    root.location = (*xy, 0)
    root.rotation_euler.z = math.radians(angle)
    root['category'] = category
    root['stage'] = 'PRIMARY_MASS_ONLY'
    root['animation'] = 'Static at stage 01; moving parts/pivots deferred until structure gate.'
    assets.append(root)
    return root

def adopt(obj, name, material, architectural=False):
    obj.name = name
    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)
    (envelope if architectural else current).objects.link(obj)
    if not architectural:
        obj.parent = root
    obj.data.materials.append(M[material])
    return obj

def bevel(obj, width, segments=3):
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    if width:
        mod = obj.modifiers.new('PrimaryEdgeRadius', 'BEVEL')
        mod.width = width
        mod.segments = segments
    mod = obj.modifiers.new('BroadFaceNormals', 'WEIGHTED_NORMAL')
    mod.keep_sharp = True
    return obj

def box(name, loc, size, material, radius=0, architectural=False):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    adopt(obj, name, material, architectural)
    return bevel(obj, radius)

def taper(name, loc, radius1, radius2, depth, material, vertices=16):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1,
        radius2=radius2, depth=depth, location=loc)
    obj = adopt(bpy.context.object, name, material)
    return bevel(obj, .008, 2)

def blob(name, loc, size, material):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, location=loc)
    obj = bpy.context.object
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    adopt(obj, name, material)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj

def silhouette(name, points, depth, material, y=0, radius=.02):
    n = len(points)
    verts = [(x, yy, z) for yy in (y-depth/2, y+depth/2) for x, z in points]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    current.objects.link(obj)
    obj.parent = root
    mesh.materials.append(M[material])
    # Consistent outward normals on all authored closed masses.
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.select_set(False)
    return bevel(obj, radius)

def legs(width, depth, height, material='wood', radius=.035):
    for x in (-width/2, width/2):
        for y in (-depth/2, depth/2):
            taper('SupportMass', (x, y, height/2), radius*.68, radius, height, material)

def seating(name, xy, angle, width, depth, height, color, wing=False):
    asset(name, xy, angle)
    legs(width-.18, depth-.19, .16, radius=.037)
    box('SeatEnvelope', (0, 0, .33), (width-.12, depth-.09, .42), color, .09)
    hw = width/2-.085
    points = [(-hw, .15), (hw, .15), (hw+.015, height-.22),
              (hw-.07, height-.075), (hw*.45, height),
              (-hw*.48, height-.015), (-hw+.055, height-.10)]
    back = silhouette('BackEnvelope', points, .25, color, -depth/2+.075, .065)
    back.rotation_euler.x = math.radians(-6)
    for sign in (-1, 1):
        box('ArmSideEnvelope', (sign*(width/2-.09), -.005, .365),
            (.18, depth-.12, .47), color, .06)
        box('RolledArmEnvelope', (sign*(width/2-.085), .02, .60),
            (.20, depth-.05, .29), color, .095)
        if wing:
            blob('WingEnvelope', (sign*(width/2-.13), -depth/2+.13, height-.26),
                (.12, .15, .25), color)

def table(name, xy, width, depth, height, angle=0):
    asset(name, xy, angle)
    box('TopEnvelope', (0, 0, height-.035), (width, depth, .07), 'wood', .018)
    box('ApronEnvelope', (0, 0, height-.12), (width-.10, depth-.10, .13), 'wood', .008)
    legs(width-.17, depth-.17, height-.13, radius=.034)

def round_table(name, xy, diameter=.48, height=.57):
    asset(name, xy)
    taper('RoundTopEnvelope', (0, 0, height-.027), diameter/2, diameter/2, .055, 'wood', 32)
    for a in (0, 120, 240):
        angle = math.radians(a)
        taper('LegEnvelope', (.15*math.cos(angle), .15*math.sin(angle), (height-.05)/2),
              .023, .035, height-.05, 'wood', 12)

# Primary arrangement: back wall is +Y, window wall is +X.
box('FloorEnvelope', (0, 0, -.095), (7, 6, .18), 'floor', .015, True)
# Door gap on left of fireplace; wall beside the window retains a genuine opening.
for name, loc, size in [
    ('BackWallLeft', (-3.25, 3.05, 1.7), (.5, .18, 3.4)),
    ('BackWallMain', (.75, 3.05, 1.7), (5.5, .18, 3.4)),
    ('BackDoorLintel', (-2.5, 3.05, 2.88), (1, .18, 1.04)),
    ('RightWallBack', (3.55, 2.325, 1.7), (.18, 1.35, 3.4)),
    ('RightWallFront', (3.55, -1.825, 1.7), (.18, 2.35, 3.4)),
    ('WindowSillWall', (3.55, .50, .39), (.18, 2.3, .78)),
    ('WindowLintel', (3.55, .50, 3.26), (.18, 2.3, .28)),
]:
    box(name, loc, size, 'wall', .01, True)

asset('Rug', (0, -.05), category='SoftFurnishing')
box('RugEnvelope', (0, 0, .012), (3.85, 4.12, .024), 'rug', .012)
seating('Sofa', (.53, -1.42), 0, 2.20, .93, 1.02, 'green')
seating('ArmchairOchre', (1.34, 1.22), 165, .88, .86, 1.08, 'gold', True)
seating('ArmchairRusset', (-1.90, -.33), -95, .80, .82, 1.00, 'rust')
table('CoffeeTable', (-.05, .03), 1.25, .65, .48)
round_table('SideTableCup', (-1.32, -1.05), .47, .56)
round_table('SideTableLamp', (-.48, -2.03), .49, .59)
round_table('SideTablePlant', (3.01, -1.02), .58, .70)
table('WindowConsole', (3.04, .58), 1.8, .48, .83, 90)

asset('Ottoman', (2.18, 2.42))
legs(.58, .52, .13, radius=.025)
box('UpholsteredEnvelope', (0, 0, .32), (.72, .65, .43), 'green', .075)

asset('WoodChair', (3.02, -1.75), 90)
legs(.37, .38, .46, radius=.024)
box('SeatEnvelope', (0, 0, .47), (.47, .48, .09), 'rust', .025)
for x in (-.205, .205):
    box('BackPostEnvelope', (x, -.20, .71), (.045, .05, .63), 'wood', .008)
box('BackEnvelope', (0, -.20, .87), (.40, .06, .24), 'wood', .02)

asset('Fireplace', (-.27, 2.72), 180, 'Architecture')
box('HearthEnvelope', (0, .05, .065), (1.85, .78, .13), 'stone', .025)
for x in (-.68, .68):
    box('PierEnvelope', (x, 0, .74), (.25, .35, 1.30), 'stone', .025)
box('LintelEnvelope', (0, 0, 1.34), (1.6, .36, .22), 'stone', .025)
box('MantelEnvelope', (0, .025, 1.50), (1.76, .49, .12), 'stone', .023)
box('FireboxRecess', (0, -.15, .70), (1.12, .09, 1.12), 'dark')

asset('ArchBookcase', (1.23, 2.78), 180)
box('BaseCabinetEnvelope', (0, 0, .49), (.80, .38, .98), 'wood', .025)
arch = [(-.40, .91), (.40, .91)]
arch += [(.40*math.cos(i*math.pi/12), 2.19+.40*math.sin(i*math.pi/12)) for i in range(13)]
silhouette('ArchedUpperEnvelope', arch, .27, 'wood', -.05, .02)

asset('SmallCabinet', (-2.82, -1.92), -90)
legs(.56, .33, .12, radius=.033)
box('CabinetEnvelope', (0, 0, .46), (.70, .46, .76), 'wood', .02)
box('TopEnvelope', (0, 0, .87), (.76, .51, .07), 'wood', .02)

asset('CoatStand', (-2.26, 2.43))
taper('FootEnvelope', (0, 0, .055), .21, .16, .11, 'wood', 24)
taper('ShaftEnvelope', (0, 0, .93), .04, .027, 1.76, 'wood')
blob('CoatEnvelope', (-.015, .045, 1.05), (.16, .105, .49), 'rust')
for a in (0, 90, 180, 270):
    r = math.radians(a)
    obj = box('HookMass', (.12*math.cos(r), .12*math.sin(r), 1.64), (.25, .04, .05), 'wood', .018)
    obj.rotation_euler.z = r

asset('LogBasket', (.90, 2.13), category='Prop')
box('BasketEnvelope', (0, 0, .22), (.42, .35, .44), 'wood', .05)
asset('FireTools', (-1.29, 2.30), category='Prop')
box('RackEnvelope', (0, 0, .25), (.31, .25, .5), 'dark', .025)

asset('Window', (3.46, .50), 90, 'Architecture')
box('GlassEnvelope', (0, 0, 1.99), (2.3, .025, 2.40), 'glass')
for x in (-1.16, 0, 1.16):
    box('FrameEnvelope', (x, -.035, 1.99), (.06, .10, 2.48), 'stone', .008)
for z in (.78, 1.56, 2.34, 3.19):
    box('FrameEnvelope', (0, -.035, z), (2.38, .10, .05), 'stone', .008)
asset('Curtains', (3.31, .50), 90, 'SoftFurnishing')
for x in (-1.28, 1.28):
    box('DraperyEnvelope', (x, -.02, 1.68), (.34, .19, 3.22), 'curtain', .065)

def picture(name, xy, angle, width, height, z):
    asset(name, xy, angle, 'WallDecor')
    box('FrameEnvelope', (0, 0, z), (width, .075, height), 'wood', .012)
    box('ImagePlaceholder', (0, .043, z), (width-.07, .01, height-.07), 'wall')

picture('PaintingMantel', (-.27, 2.92), 180, 1.20, 1.50, 2.42)
picture('PaintingBackSmall', (2.11, 2.93), 180, .45, .68, 2.31)
picture('PaintingCorner', (3.43, 2.66), 90, .49, .65, 2.27)
picture('PaintingRight', (3.43, -1.28), 90, .67, .91, 2.03)
picture('PaintingLeft', (-3.02, -1.88), -90, .56, .78, 1.66)

def plant(name, xy, size, base=0):
    asset(name, xy, category='Foliage')
    taper('PotEnvelope', (0, 0, base+.15*size), .13*size, .19*size, .30*size, 'pot', 20)
    taper('StemEnvelope', (0, 0, base+.51*size), .017*size, .011*size, .53*size, 'wood', 10)
    for x, y, z, sx, sy, sz in [(-.14, 0, .76, .20, .19, .27), (.12, .07, .89, .22, .20, .30), (0, -.12, .65, .24, .20, .22)]:
        blob('CanopyEnvelope', (x*size, y*size, base+z*size), (sx*size, sy*size, sz*size), 'leaf')

plant('Palm', (2.93, -.53), 1.30)
plant('ForegroundPlant', (2.70, -2.55), .96)
plant('ConsolePlant', (3.03, 1.25), .53, .83)
plant('CabinetPlant', (-2.83, -1.65), .46, .91)

def lamp(name, xy, base, height=.52):
    asset(name, xy, category='LightingProp')
    taper('BaseEnvelope', (0, 0, base+.025), .085, .08, .05, 'wood', 24)
    taper('StemEnvelope', (0, 0, base+height*.35), .035, .02, height*.60, 'wood', 16)
    taper('ShadeEnvelope', (0, 0, base+height*.78), height*.28, height*.135, height*.40, 'shade', 24)

lamp('ConsoleLamp', (3.03, -.09), .83)
lamp('CabinetLamp', (-2.82, -2.10), .91)
lamp('SofaLamp', (-.48, -2.03), .59, .39)
lamp('WallLampLeft', (-1.34, 2.74), 1.71, .38)
lamp('WallLampRight', (.61, 2.74), 2.06, .38)

# Neutral presentation: plain masses, no image-derived lighting baked in.
world = bpy.data.worlds.new('S01_NeutralWorld')
world.use_nodes = True
background = next(n for n in world.node_tree.nodes if n.type == 'BACKGROUND')
background.inputs[0].default_value = (.52, .58, .67, 1)
background.inputs[1].default_value = .45
scene.world = world
def light(name, loc, energy, size, color):
    data = bpy.data.lights.new(name, 'AREA')
    data.energy, data.shape, data.size, data.color = energy, 'DISK', size, color
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector((0, 0, 0))-obj.location).to_track_quat('-Z', 'Y').to_euler()
light('S01_Key', (-3, -2, 8), 1600, 6, (1.0, .89, .75))
light('S01_Fill', (3, 1, 6), 1100, 5, (.78, .86, 1.0))

cameras = {
    'Reference': ((-9, -12, 12.1), (0, .25, 1.05), 9.8),
    'Reverse': ((9, 11, 10), (0, 0, .6), 9.2),
    'Top': ((0, 0, 15), (0, 0, 0), 8.0),
    'Front': ((0, -15, 3.8), (0, .3, 1.5), 8.1),
}
for name, (loc, target, scale) in cameras.items():
    data = bpy.data.cameras.new('S01_' + name)
    data.type, data.ortho_scale = 'ORTHO', scale
    obj = bpy.data.objects.new('S01_' + name, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z', 'Y').to_euler()
scene.camera = scene.objects['S01_Reference']
scene.render.engine = 'CYCLES'
scene.cycles.samples = 24
scene.cycles.use_denoising = True
scene.render.resolution_x = 1200
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.view_settings.view_transform = 'AgX'
scene.render.film_transparent = False

# Self-contained original reference; never treated as scene instructions.
ref = bpy.data.images.load(scene['source'], check_existing=True)
ref.use_fake_user = True
ref.pack()
scene['packed_reference'] = ref.name
scene['render_note'] = 'Blender inspection cameras, not verified UE Gameplay Camera.'
bpy.context.view_layer.update()
manifest = []
depsgraph = bpy.context.evaluated_depsgraph_get()
for parent in assets:
    meshes = [o for o in parent.children if o.type == 'MESH']
    corners = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    bounds = [round(max(c[i] for c in corners)-min(c[i] for c in corners), 4) for i in range(3)]
    tris = 0
    for obj in meshes:
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        tris += len(mesh.loop_triangles)
        evaluated.to_mesh_clear()
    manifest.append({'asset': parent.name, 'category': parent['category'],
        'mesh_count': len(meshes), 'evaluated_triangles': tris,
        'world_bounds_m': bounds, 'origin_m': list(parent.location)})
(OUT / 'manifest.json').write_text(json.dumps({'stage':'01', 'status':'AWAITING_VISUAL_REVIEW',
    'assets':manifest, 'fbx_status':'DEFERRED_UNTIL_FINAL_GATE'}, ensure_ascii=False, indent=2), encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'LivingRoom_Stage01.blend'))
print(json.dumps({'scene':scene.name, 'asset_count':len(assets), 'mesh_count':sum(a['mesh_count'] for a in manifest),
    'evaluated_triangles':sum(a['evaluated_triangles'] for a in manifest), 'blend':str(OUT / 'LivingRoom_Stage01.blend')}))
