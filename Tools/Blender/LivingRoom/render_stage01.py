"""Inspection views only. Load through MCP, then call render_view / render_asset."""
import bpy
import json
import math
import bmesh
from pathlib import Path
from mathutils import Vector

OUT = Path('D:/25DGame/LostRunic/ArtSource/LivingRoom/Stage01')
s = bpy.data.scenes['LR_LivingRoom_Stage01']
bpy.context.window.scene = s
normal_world = s.world
normal_transform = s.view_settings.view_transform

def reset_view():
    s.world = normal_world
    s.view_settings.view_transform = normal_transform
    for obj in s.objects:
        obj.hide_render = False
    s.camera = s.objects['S01_Reference']
    s.view_layers[0].material_override = None
    s.render.resolution_x, s.render.resolution_y = 1200, 1000
    # Left/front walls are deliberately cut away. Suppress a floating wall picture.
    for obj in s.objects['ROOT_PaintingLeft'].children:
        obj.hide_render = True

def render_view(name, filename, gray=False):
    reset_view()
    s.camera = s.objects['S01_' + name]
    if name == 'Reverse':
        for obj in s.objects:
            if obj.type == 'MESH' and obj.parent is None and obj.name != 'FloorEnvelope':
                obj.hide_render = True
        for parent in [o for o in s.objects if o.type == 'EMPTY']:
            if parent.name in ('ROOT_Window', 'ROOT_Curtains') or parent.get('category') == 'WallDecor':
                for obj in parent.children:
                    obj.hide_render = True
    if gray:
        m = bpy.data.materials.get('S01_InspectionClay')
        if not m:
            m = bpy.data.materials.new('S01_InspectionClay')
            m.diffuse_color = (.42, .42, .42, 1)
        s.view_layers[0].material_override = m
    s.render.filepath = str(OUT / filename)
    bpy.ops.render.render(write_still=True)
    reset_view()
    print('Rendered ' + filename)

def render_asset(name, filename, reverse=False, silhouette=False, side=False):
    reset_view()
    parent = s.objects['ROOT_' + name]
    for obj in s.objects:
        if obj.type == 'MESH' and obj.parent != parent and obj.name != 'FloorEnvelope':
            obj.hide_render = True
    corners = [obj.matrix_world @ Vector(c) for obj in parent.children
        if obj.type == 'MESH' for c in obj.bound_box]
    lo = Vector([min(v[i] for v in corners) for i in range(3)])
    hi = Vector([max(v[i] for v in corners) for i in range(3)])
    target = (lo+hi)/2
    cam = s.objects.get('S01_AssetDetail')
    if not cam:
        data = bpy.data.cameras.new('S01_AssetDetail')
        data.type = 'ORTHO'
        cam = bpy.data.objects.new('S01_AssetDetail', data)
        s.collection.objects.link(cam)
    offset = Vector((3, -5 if reverse else 5, 2.7))
    if silhouette:
        offset = Vector((6, 0, 0)) if side else Vector((0, 6, 0))
    cam.location = target + parent.rotation_euler.to_matrix() @ offset
    cam.rotation_euler = (target-cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = max((hi-lo).length*1.08, 1.65)
    s.camera = cam
    s.render.resolution_x = 900
    s.render.resolution_y = 750
    if silhouette:
        s.objects['FloorEnvelope'].hide_render = True
        s.render.resolution_x, s.render.resolution_y = 600, 500
        cam.data.ortho_scale = max(cam.data.ortho_scale, (hi.z-lo.z)*1.2*1.15)
        black = bpy.data.materials.get('S01_BlackSilhouette')
        if not black:
            black = bpy.data.materials.new('S01_BlackSilhouette')
            black.use_nodes = True
            black.node_tree.nodes.clear()
            output = black.node_tree.nodes.new('ShaderNodeOutputMaterial')
            emission = black.node_tree.nodes.new('ShaderNodeEmission')
            emission.inputs['Color'].default_value = (0, 0, 0, 1)
            black.node_tree.links.new(emission.outputs[0], output.inputs['Surface'])
        white = bpy.data.worlds.get('S01_SilhouetteWhite')
        if not white:
            white = bpy.data.worlds.new('S01_SilhouetteWhite')
            white.use_nodes = True
            bg = next(n for n in white.node_tree.nodes if n.type == 'BACKGROUND')
            bg.inputs[0].default_value = (1, 1, 1, 1)
            bg.inputs[1].default_value = 1
        s.world = white
        s.view_settings.view_transform = 'Standard'
        s.view_layers[0].material_override = black
    s.render.filepath = str(OUT / filename)
    bpy.ops.render.render(write_still=True)
    reset_view()
    print('Rendered ' + filename)

def validate_stage():
    issues = []
    meshes = [o for o in s.objects if o.type == 'MESH']
    for obj in meshes:
        if any(not math.isfinite(v) for row in obj.matrix_world for v in row):
            issues.append({'object':obj.name, 'issue':'nonfinite_transform'})
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        if any(not e.is_manifold for e in bm.edges):
            issues.append({'object':obj.name, 'issue':'nonmanifold_base_mass'})
        if any(f.calc_area() < 1e-9 for f in bm.faces):
            issues.append({'object':obj.name, 'issue':'degenerate_face'})
        bm.free()
    def extent_y(parent):
        values = [(obj.matrix_world @ Vector(c)).y for obj in s.objects[parent].children
            if obj.type == 'MESH' for c in obj.bound_box]
        return min(values), max(values)
    painting_low, _ = extent_y('ROOT_PaintingCorner')
    _, curtain_high = extent_y('ROOT_Curtains')
    gap = painting_low-curtain_high
    if gap <= 0:
        issues.append({'object':'PaintingCorner/Curtains', 'issue':'wall_span_overlap'})
    data = {'stage':'01_PRIMARY_MASSES', 'mesh_count_including_architecture':len(meshes),
        'checks':['finite transforms', 'closed primitive masses', 'nonzero face area'],
        'issues':issues, 'technical_status':'PASS' if not issues else 'FAIL',
        'visual_status':'AWAITING_USER', 'fbx_roundtrip':'NOT_APPLICABLE_YET',
        'uv_material_animation':'DEFERRED_TO_LATER_GATES',
        'workflow':'Docs/Art/02_ConceptDrivenBlenderWorkflow.md',
        'previous_visual_gate':'FAIL_USER_LAYOUT_FEEDBACK',
        'corner_painting_to_curtain_gap_m':round(gap, 4),
        'silhouette_views':[p.name for p in sorted(OUT.glob('Silhouette_*.png'))]}
    (OUT/'validation.json').write_text(json.dumps(data, indent=2), encoding='utf-8')
    print(json.dumps(data))

def save_review():
    reset_view()
    for area in bpy.context.screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces.active.region_3d.view_perspective = 'CAMERA'
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'LivingRoom_Stage01.blend'))
