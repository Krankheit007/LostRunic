"""Generate deterministic Blender-authored M1 foliage FBX prototypes.

Run with Blender:
    blender --background --python generate_m1_foliage_meshes.py
"""

from pathlib import Path
import math

import bpy
from mathutils import Vector


OUTPUT_DIR = Path(__file__).resolve().parent / "Generated" / "M1Foliage"
NEUTRAL_RGB = (0.5, 0.5, 0.5, 1.0)


def _reset() -> None:
	bpy.ops.object.select_all(action="SELECT")
	bpy.ops.object.delete(use_global=False)
	for datablock in bpy.data.meshes:
		bpy.data.meshes.remove(datablock)


def _material(name: str) -> bpy.types.Material:
	material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
	return material


def _append_face(faces, face_uvs, material_indices, indices, uvs, material_index):
	faces.append(indices)
	face_uvs.append(uvs)
	material_indices.append(material_index)


def _append_cylinder(vertices, normals, colors, alphas, faces, face_uvs, material_indices, start, end, radius_a, radius_b, segments=10):
	start = Vector(start)
	end = Vector(end)
	axis = (end - start).normalized()
	reference = Vector((0.0, 0.0, 1.0)) if abs(axis.z) < 0.9 else Vector((1.0, 0.0, 0.0))
	right = axis.cross(reference).normalized()
	up = right.cross(axis).normalized()
	base = len(vertices)
	for ring, (center, radius) in enumerate(((start, radius_a), (end, radius_b))):
		for index in range(segments):
			angle = index * math.tau / segments
			radial = (right * math.cos(angle) + up * math.sin(angle)).normalized()
			vertices.append(tuple(center + radial * radius))
			normals.append(tuple(radial))
			colors.append(NEUTRAL_RGB)
			alphas.append(0.0)
	for index in range(segments):
		next_index = (index + 1) % segments
		_append_face(
			faces,
			face_uvs,
			material_indices,
			(base + index, base + next_index, base + segments + next_index, base + segments + index),
			((index / segments, 0.0), (next_index / segments, 0.0), (next_index / segments, 1.0), (index / segments, 1.0)),
			0,
		)


def _append_cluster(vertices, normals, colors, alphas, faces, face_uvs, material_indices, center, lobe_center, size, angle, uv_rect, mass_color):
	center = Vector(center)
	lobe_center = Vector(lobe_center)
	volume_normal = Vector((
		(center.x - lobe_center.x) / 1.15,
		(center.y - lobe_center.y) / 0.95,
		(center.z - lobe_center.z) / 0.82,
	)).normalized()
	reference = Vector((0.0, 0.0, 1.0)) if abs(volume_normal.z) < 0.92 else Vector((1.0, 0.0, 0.0))
	right = volume_normal.cross(reference).normalized()
	up = right.cross(volume_normal).normalized()
	right = (right * math.cos(angle) + up * math.sin(angle)).normalized()
	up = right.cross(volume_normal).normalized()
	shape = ((-0.62, -0.28), (-0.42, -0.58), (0.0, -0.7), (0.5, -0.48), (0.66, -0.05), (0.46, 0.5), (0.0, 0.68), (-0.52, 0.46))
	uv_shape = ((0.08, 0.30), (0.18, 0.08), (0.50, 0.02), (0.82, 0.12), (0.94, 0.47), (0.80, 0.84), (0.50, 0.96), (0.14, 0.82))
	base = len(vertices)
	# RGB is explicitly initialized to neutral before the lobe mass paint is applied.
	for point in shape:
		position = center + right * point[0] * size + up * point[1] * size
		vertices.append(tuple(position))
		ellipsoid_normal = Vector((
			(position.x - lobe_center.x) / 1.15,
			(position.y - lobe_center.y) / 0.95,
			(position.z - lobe_center.z) / 0.82,
		)).normalized()
		normals.append(tuple(ellipsoid_normal))
		colors.append(NEUTRAL_RGB)
		colors[-1] = (*mass_color, 1.0)
		alphas.append(0.0)
	u0, v0, u1, v1 = uv_rect
	uvs = tuple((u0 + uv[0] * (u1 - u0), v0 + uv[1] * (v1 - v0)) for uv in uv_shape)
	for index in range(1, 7):
		_append_face(
			faces,
			face_uvs,
			material_indices,
			(base, base + index, base + index + 1),
			(uvs[0], uvs[index], uvs[index + 1]),
			1,
		)


def _build_object(name, vertices, normals, colors, alphas, faces, face_uvs, material_indices, materials):
	mesh = bpy.data.meshes.new(name)
	# UE uses centimeters and the MCP FBX importer preserves numeric vertex
	# coordinates, so write explicit centimeter geometry instead of relying on
	# FBX unit metadata or compensating with Actor Scale.
	centimeter_vertices = [(x * 100.0, y * 100.0, z * 100.0) for x, y, z in vertices]
	mesh.from_pydata(centimeter_vertices, [], faces)
	mesh.update()
	for material in materials:
		mesh.materials.append(material)
	for polygon, material_index in zip(mesh.polygons, material_indices):
		polygon.material_index = material_index
	uv_layer = mesh.uv_layers.new(name="UVMap")
	for polygon, uvs in zip(mesh.polygons, face_uvs):
		for loop_index, uv in zip(polygon.loop_indices, uvs):
			uv_layer.data[loop_index].uv = uv
	color_layer = mesh.color_attributes.new(name="Color", type="FLOAT_COLOR", domain="POINT")
	for index, color in enumerate(colors):
		color_layer.data[index].color = (color[0], color[1], color[2], alphas[index])
	loop_normals = [normals[loop.vertex_index] for loop in mesh.loops]
	mesh.normals_split_custom_set(loop_normals)
	obj = bpy.data.objects.new(name, mesh)
	bpy.context.collection.objects.link(obj)
	return obj


def _collision_cylinder(name, radius=36.0, height=305.0, segments=10):
	vertices = []
	faces = []
	for z in (0.0, height):
		for index in range(segments):
			angle = index * math.tau / segments
			vertices.append((math.cos(angle) * radius, math.sin(angle) * radius, z))
	for index in range(segments):
		next_index = (index + 1) % segments
		faces.append((index, next_index, segments + next_index, segments + index))
	faces.append(tuple(range(segments - 1, -1, -1)))
	faces.append(tuple(range(segments, segments * 2)))
	mesh = bpy.data.meshes.new(name)
	mesh.from_pydata(vertices, [], faces)
	mesh.update()
	obj = bpy.data.objects.new(name, mesh)
	bpy.context.collection.objects.link(obj)
	return obj


def _export(obj, filename, collision=None):
	bpy.ops.object.select_all(action="DESELECT")
	obj.select_set(True)
	if collision is not None:
		collision.select_set(True)
	bpy.context.view_layer.objects.active = obj
	bpy.ops.export_scene.fbx(
		filepath=str(OUTPUT_DIR / filename),
		use_selection=True,
		apply_unit_scale=True,
		apply_scale_options="FBX_SCALE_UNITS",
		axis_forward="-Y",
		axis_up="Z",
		use_mesh_modifiers=True,
		mesh_smooth_type="FACE",
		use_tspace=True,
		add_leaf_bones=False,
		bake_anim=False,
	)
	bpy.data.objects.remove(obj, do_unlink=True)
	if collision is not None:
		bpy.data.objects.remove(collision, do_unlink=True)


def generate_tree():
	vertices, normals, colors, alphas, faces, face_uvs, material_indices = [], [], [], [], [], [], []
	_append_cylinder(vertices, normals, colors, alphas, faces, face_uvs, material_indices, (0, 0, 0), (0, 0, 3.0), 0.32, 0.17, 12)
	for endpoint in ((-1.25, 0.0, 3.45), (1.1, 0.15, 3.65), (0.2, -0.75, 4.05), (-0.3, 0.7, 4.2)):
		_append_cylinder(vertices, normals, colors, alphas, faces, face_uvs, material_indices, (0, 0, 2.15), endpoint, 0.13, 0.055, 8)
	lobes = (
		((-1.15, 0.05, 3.75), (1.25, 0.9, 0.95), (0.42, 0.50, 0.58), (0.024, 0.024, 0.488, 0.488)),
		((1.05, 0.12, 3.9), (1.2, 0.95, 1.0), (0.56, 0.55, 0.46), (0.512, 0.024, 0.976, 0.488)),
		((-0.2, -0.55, 4.55), (1.25, 0.85, 0.9), (0.63, 0.58, 0.45), (0.024, 0.024, 0.488, 0.488)),
		((0.05, 0.62, 4.15), (1.35, 0.9, 1.05), (0.44, 0.49, 0.57), (0.512, 0.024, 0.976, 0.488)),
	)
	golden_angle = math.pi * (3.0 - math.sqrt(5.0))
	for lobe_index, (center, radii, mass_color, uv_rect) in enumerate(lobes):
		for index in range(20):
			y = 1.0 - (index / 19.0) * 2.0
			radial = math.sqrt(max(0.0, 1.0 - y * y))
			theta = golden_angle * index + lobe_index * 0.55
			# Every third card sits inside the outer shell. The layered distribution
			# fills the lobe as a color mass without relying on giant empty quads.
			shell = 0.58 if index % 3 == 0 else 0.92
			position = (
				center[0] + math.cos(theta) * radial * radii[0] * shell,
				center[1] + math.sin(theta) * radial * radii[1] * shell,
				center[2] + y * radii[2] * shell,
			)
			_append_cluster(vertices, normals, colors, alphas, faces, face_uvs, material_indices, position, center, 0.96, theta * 0.37, uv_rect, mass_color)
	obj = _build_object("SM_LR_M1_Tree_Medium_A", vertices, normals, colors, alphas, faces, face_uvs, material_indices, (_material("Trunk"), _material("Canopy")))
	collision = _collision_cylinder("UCX_SM_LR_M1_Tree_Medium_A_00")
	_export(obj, "SM_LR_M1_Tree_Medium_A.fbx", collision)


def _generate_grass(name, blade_count, hero, seed_phase):
	vertices, normals, colors, alphas, faces, face_uvs, material_indices = [], [], [], [], [], [], []
	uv_rect = (0.024, 0.512, 0.488, 0.976) if not hero else (0.512, 0.512, 0.976, 0.976)
	for blade in range(blade_count):
		angle = seed_phase + blade * math.tau / blade_count
		radius = 0.12 + 0.13 * ((blade * 7) % 5) / 4.0
		root = Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0.0))
		height = (1.0 if hero else 0.72) * (0.78 + 0.07 * (blade % 4))
		width = (0.18 if hero else 0.14) * (0.85 + 0.05 * (blade % 3))
		right = Vector((-math.sin(angle), math.cos(angle), 0.0))
		forward = Vector((math.cos(angle), math.sin(angle), 0.0))
		base = len(vertices)
		for row in range(4):
			t = row / 3.0
			center = root + forward * (0.16 * t * t) + Vector((0, 0, height * t))
			half_width = width * (1.0 - 0.72 * t)
			for side in (-1.0, 1.0):
				vertices.append(tuple(center + right * half_width * side))
				normals.append(tuple((forward * 0.35 + Vector((0, 0, 0.94))).normalized()))
				colors.append(NEUTRAL_RGB)
				mass = 0.43 + 0.15 * t
				colors[-1] = (mass, 0.48 + 0.1 * t, 0.44 + 0.06 * t, 1.0)
				alphas.append(t)
		for row in range(3):
			u0 = uv_rect[0]
			u1 = uv_rect[2]
			v0 = uv_rect[1] + (uv_rect[3] - uv_rect[1]) * row / 3.0
			v1 = uv_rect[1] + (uv_rect[3] - uv_rect[1]) * (row + 1) / 3.0
			_append_face(faces, face_uvs, material_indices, (base + row * 2, base + row * 2 + 1, base + (row + 1) * 2 + 1, base + (row + 1) * 2), ((u0, v0), (u1, v0), (u1, v1), (u0, v1)), 0)
	obj = _build_object(name, vertices, normals, colors, alphas, faces, face_uvs, material_indices, (_material("Grass"),))
	_export(obj, f"{name}.fbx")


def generate():
	OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
	_reset()
	bpy.context.scene.unit_settings.system = "METRIC"
	bpy.context.scene.unit_settings.scale_length = 1.0
	generate_tree()
	_generate_grass("SM_LR_M1_GrassClump_A", 8, False, 0.0)
	_generate_grass("SM_LR_M1_GrassClump_B", 10, False, 0.31)
	_generate_grass("SM_LR_M1_GrassClump_C", 12, False, 0.67)
	_generate_grass("SM_LR_M1_HeroTuft_A", 15, True, 0.21)


if __name__ == "__main__":
	generate()
