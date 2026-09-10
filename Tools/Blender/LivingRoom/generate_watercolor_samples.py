"""Generate deterministic watercolor texture candidates for the LivingRoom sources.

This is a source-side sampler. It writes only the files named by the JSON config
under ArtSource/LivingRoom/WatercolorSamples and never imports or edits UE assets.
Pillow is used for broad, hand-authored shapes; no random or photographic noise is
introduced. A generated image is a candidate and still requires visual review.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps


ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CONFIG = Path(__file__).with_name("watercolor_samples_config.json")
GENERATORS = {"cool_plaster", "floor_mass", "wood", "rug"}


def read_json(path: Path) -> Any:
	return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: Any) -> None:
	path.parent.mkdir(parents=True, exist_ok=True)
	path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def repo_path(relative_path: str) -> Path:
	return ROOT / relative_path.replace("/", "\\")


def relative_path(path: Path) -> str:
	return path.relative_to(ROOT).as_posix()


def parse_color(value: str) -> tuple[int, int, int]:
	value = value.strip().lstrip("#")
	if len(value) != 6:
		raise ValueError(f"Expected #RRGGBB color, got {value!r}")
	return tuple(int(value[index:index + 2], 16) for index in (0, 2, 4))


def opacity_byte(value: float | int) -> int:
	value = float(value)
	if value <= 1.0:
		value *= 255.0
	return max(0, min(255, int(round(value))))


def pixel_point(point: Iterable[float], size: tuple[int, int], origin: tuple[int, int] = (0, 0)) -> tuple[int, int]:
	values = list(point)
	return (origin[0] + int(round(values[0] * size[0])), origin[1] + int(round(values[1] * size[1])))


def expanded_size(size: tuple[int, int]) -> tuple[int, int]:
	return size[0] * 3, size[1] * 3


def center_crop(image: Image.Image, size: tuple[int, int]) -> Image.Image:
	w, h = size
	return image.crop((w, h, w * 2, h * 2))


def expanded_canvas(image: Image.Image, size: tuple[int, int]) -> Image.Image:
	if image.size == expanded_size(size):
		return image
	canvas = Image.new(image.mode, expanded_size(size))
	for tile_y in (-1, 0, 1):
		for tile_x in (-1, 0, 1):
			canvas.paste(image, (size[0] * (tile_x + 1), size[1] * (tile_y + 1)))
	return canvas


def draw_shape(draw: ImageDraw.ImageDraw, wash: dict[str, Any], size: tuple[int, int], origin: tuple[int, int], fill: Any) -> None:
	shape = wash.get("shape", "ellipse")
	if shape == "ellipse":
		center = pixel_point(wash["center"], size, origin)
		radius = pixel_point(wash["radius"], size)
		box = (center[0] - radius[0], center[1] - radius[1], center[0] + radius[0], center[1] + radius[1])
		draw.ellipse(box, fill=fill)
		return
	if shape == "polygon":
		points = [pixel_point(point, size, origin) for point in wash["points"]]
		draw.polygon(points, fill=fill)
		return
	raise ValueError(f"Unsupported watercolor shape: {shape}")


def apply_wrapped_shapes(
	base: Image.Image,
	size: tuple[int, int],
	washes: list[dict[str, Any]],
	softness: int,
	value_mode: bool = False,
) -> Image.Image:
	base = expanded_canvas(base, size)
	overlay = Image.new("RGBA", expanded_size(size), (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	for wash in washes:
		if value_mode:
			value = int(round(max(0.0, min(1.0, float(wash["value"]))) * 255.0))
			fill = (value, value, value, opacity_byte(wash.get("opacity", 1.0)))
		else:
			fill = (*parse_color(wash["color"]), opacity_byte(wash.get("opacity", 1.0)))
		for tile_y in (-1, 0, 1):
			for tile_x in (-1, 0, 1):
				draw_shape(draw, wash, size, ((tile_x + 1) * size[0], (tile_y + 1) * size[1]), fill)
	if softness > 0:
		overlay = overlay.filter(ImageFilter.GaussianBlur(softness))
	result = Image.alpha_composite(base.convert("RGBA"), overlay)
	return center_crop(result, size).convert("RGB")


def apply_unique_shapes(
	base: Image.Image,
	size: tuple[int, int],
	washes: list[dict[str, Any]],
	softness: int,
	value_mode: bool = False,
) -> Image.Image:
	overlay = Image.new("RGBA", size, (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	for wash in washes:
		if value_mode:
			value = int(round(max(0.0, min(1.0, float(wash["value"]))) * 255.0))
			fill = (value, value, value, opacity_byte(wash.get("opacity", 1.0)))
		else:
			fill = (*parse_color(wash["color"]), opacity_byte(wash.get("opacity", 1.0)))
		draw_shape(draw, wash, size, (0, 0), fill)
	if softness > 0:
		overlay = overlay.filter(ImageFilter.GaussianBlur(softness))
	return Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")


def make_channel(
	size: tuple[int, int],
	base_value: float,
	washes: list[dict[str, Any]],
	softness: int,
	seamless: bool,
) -> Image.Image:
	fill = int(round(max(0.0, min(1.0, base_value)) * 255.0))
	canvas_size = expanded_size(size) if seamless else size
	base = Image.new("L", canvas_size, fill)
	if not washes:
		return center_crop(base, size) if seamless else base
	overlay = Image.new("RGBA", canvas_size, (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	for wash in washes:
		value = int(round(max(0.0, min(1.0, float(wash["value"]))) * 255.0))
		fill_rgba = (value, value, value, opacity_byte(wash.get("opacity", 1.0)))
		origins = [(0, 0)]
		if seamless:
			origins = [((tx + 1) * size[0], (ty + 1) * size[1]) for ty in (-1, 0, 1) for tx in (-1, 0, 1)]
		for origin in origins:
			draw_shape(draw, wash, size, origin, fill_rgba)
	if softness > 0:
		overlay = overlay.filter(ImageFilter.GaussianBlur(softness))
	result = Image.alpha_composite(base.convert("RGBA"), overlay).convert("L")
	return center_crop(result, size) if seamless else result


def add_wrapped_lines(
	base: Image.Image,
	size: tuple[int, int],
	lines: list[dict[str, Any]],
	color: tuple[int, int, int] | int,
	default_width: int = 1,
) -> Image.Image:
	base = expanded_canvas(base, size)
	overlay = Image.new("RGBA", expanded_size(size), (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	for line in lines:
		orientation = line["orientation"]
		width = max(1, int(line.get("width_px", default_width)))
		fill_value = color if isinstance(color, int) else color
		fill = (*fill_value, opacity_byte(line.get("opacity", 1.0))) if isinstance(fill_value, tuple) else (fill_value, fill_value, fill_value, opacity_byte(line.get("opacity", 1.0)))
		position = float(line["position"])
		if orientation == "horizontal":
			for tile_y in (-1, 0, 1):
				y = int(round((tile_y + 1 + position) * size[1]))
				draw.line((-size[0], y, size[0] * 4, y), fill=fill, width=width)
		elif orientation == "vertical":
			for tile_x in (-1, 0, 1):
				x = int(round((tile_x + 1 + position) * size[0]))
				draw.line((x, -size[1], x, size[1] * 4), fill=fill, width=width)
		else:
			raise ValueError(f"Unsupported line orientation: {orientation}")
	result = Image.alpha_composite(base.convert("RGBA"), overlay)
	return center_crop(result, size).convert(base.mode)


def apply_wrapped_color_washes(base: Image.Image, size: tuple[int, int], washes: list[dict[str, Any]], softness: int) -> Image.Image:
	return apply_wrapped_shapes(base, size, washes, softness)


def draw_board_rows(size: tuple[int, int], params: dict[str, Any]) -> Image.Image:
	w, h = size
	canvas = Image.new("RGB", expanded_size(size), parse_color(params["base"]))
	draw = ImageDraw.Draw(canvas)
	for tile_y in (-1, 0, 1):
		for row in params["rows"]:
			y0 = int(round((tile_y + 1 + row["y0"]) * h))
			y1 = int(round((tile_y + 1 + row["y1"]) * h))
			bounds = [0.0, *row["segments"], 1.0]
			for index, (left, right) in enumerate(zip(bounds, bounds[1:])):
				fill = parse_color(row["fills"][index % len(row["fills"])])
				for tile_x in (-1, 0, 1):
					x0 = int(round((tile_x + row["x_offset"] + left) * w))
					x1 = int(round((tile_x + row["x_offset"] + right) * w))
					draw.rectangle((x0, y0, x1, y1), fill=fill)
	return canvas


def draw_wrapped_strokes(base: Image.Image, size: tuple[int, int], strokes: list[dict[str, Any]], softness: int) -> Image.Image:
	w, h = size
	base = expanded_canvas(base, size)
	overlay = Image.new("RGBA", expanded_size(size), (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	step = max(4, w // 80)
	for stroke in strokes:
		points = []
		for x in range(-w, w * 2 + step, step):
			x_norm = x / float(w)
			wave = math.sin((x_norm + float(stroke["phase"])) * math.tau * float(stroke["cycles"]))
			points.append((x + w, float(stroke["y"]) * h + wave * float(stroke["amplitude"]) * h))
		fill = (*parse_color(stroke["color"]), opacity_byte(stroke.get("opacity", 1.0)))
		for tile_y in (-1, 0, 1):
			shifted = [(x, y + (tile_y + 1) * h) for x, y in points]
			draw.line(shifted, fill=fill, width=max(1, int(stroke["width_px"])), joint="curve")
	if softness > 0:
		overlay = overlay.filter(ImageFilter.GaussianBlur(softness))
	return center_crop(Image.alpha_composite(base.convert("RGBA"), overlay), size).convert("RGB")


def draw_wrapped_gray_strokes(
	base: Image.Image,
	size: tuple[int, int],
	strokes: list[dict[str, Any]],
	value: float,
	opacity: float | int,
	softness: int,
) -> Image.Image:
	w, h = size
	base = expanded_canvas(base, size)
	overlay = Image.new("RGBA", expanded_size(size), (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	step = max(4, w // 80)
	gray = int(round(max(0.0, min(1.0, value)) * 255.0))
	fill = (gray, gray, gray, opacity_byte(opacity))
	for stroke in strokes:
		points = []
		for x in range(-w, w * 2 + step, step):
			x_norm = x / float(w)
			wave = math.sin((x_norm + float(stroke["phase"])) * math.tau * float(stroke["cycles"]))
			points.append((x + w, float(stroke["y"]) * h + wave * float(stroke["amplitude"]) * h))
		for tile_y in (-1, 0, 1):
			shifted = [(x, y + (tile_y + 1) * h) for x, y in points]
			draw.line(shifted, fill=fill, width=max(1, int(stroke["width_px"])), joint="curve")
	if softness > 0:
		overlay = overlay.filter(ImageFilter.GaussianBlur(softness))
	return center_crop(Image.alpha_composite(base.convert("RGBA"), overlay), size).convert("L")


def render_smk(
	size: tuple[int, int],
	params: dict[str, Any],
	seamless: bool,
	line_color: int | None = None,
	extra_r_washes: list[dict[str, Any]] | None = None,
) -> Image.Image:
	smk = params["smk"]
	r = make_channel(size, smk["r_base"], smk.get("r_washes", []), 4, seamless)
	if smk.get("r_strokes") and params.get("strokes"):
		r = draw_wrapped_gray_strokes(r, size, params["strokes"], smk.get("r_stroke_value", 0.38), smk.get("r_stroke_opacity", 145), 4)
	if extra_r_washes:
		extra = make_channel(size, 0.0, extra_r_washes, 2, seamless)
		r = ImageChops.lighter(r, extra)
	g = make_channel(size, smk["g_base"], [], 0, seamless)
	b = make_channel(size, smk["b_base"], smk.get("b_washes", []), 24, seamless)
	if smk.get("lines"):
		g = add_wrapped_lines(g, size, smk["lines"], line_color if line_color is not None else 0)
	return Image.merge("RGB", (r, g, b))


def render_cool_plaster(family: dict[str, Any]) -> tuple[Image.Image, Image.Image]:
	params = family["render_parameters"]
	size = tuple(family["output"]["dimensions_px"])
	base = Image.new("RGB", expanded_size(size), parse_color(params["base"]))
	base = apply_wrapped_shapes(base, size, params["washes"], params["softness_px"])
	smk = render_smk(size, params, True)
	return base, smk


def render_floor_mass(family: dict[str, Any]) -> tuple[Image.Image, Image.Image]:
	params = family["render_parameters"]
	size = tuple(family["output"]["dimensions_px"])
	base = draw_board_rows(size, params)
	base = apply_wrapped_color_washes(base, size, params["washes"], params["softness_px"])
	seam_color = parse_color(params["seam_color"])
	seam_lines = [{"orientation": "horizontal", "position": row["y0"], "width_px": params["seam_width_px"], "opacity": params.get("seam_opacity", 255)} for row in params["rows"] if row["y0"] > 0]
	base = add_wrapped_lines(base, size, seam_lines, seam_color)
	smk = render_smk(size, params, True, line_color=0)
	return base, smk


def render_wood(family: dict[str, Any]) -> tuple[Image.Image, Image.Image]:
	params = family["render_parameters"]
	size = tuple(family["output"]["dimensions_px"])
	base = Image.new("RGB", expanded_size(size), parse_color(params["base"]))
	base = apply_wrapped_shapes(base, size, params["washes"], params["softness_px"])
	base = draw_wrapped_strokes(base, size, params["strokes"], params["stroke_softness_px"])
	smk = render_smk(size, params, True)
	return base, smk


def draw_rug_motif(draw: ImageDraw.ImageDraw, motif: dict[str, Any], size: tuple[int, int], fill: Any) -> None:
	cx, cy = pixel_point(motif["center"], size)
	radius = max(1, int(round(float(motif["size"]) * min(size))))
	if motif["shape"] == "diamond":
		draw.polygon(((cx, cy - radius), (cx + radius, cy), (cx, cy + radius), (cx - radius, cy)), fill=fill)
		return
	if motif["shape"] == "leaf":
		draw.polygon(((cx, cy - radius), (cx + radius, cy), (cx, cy + radius), (cx - radius, cy)), fill=fill)
		return
	raise ValueError(f"Unsupported rug motif: {motif['shape']}")


def render_rug(family: dict[str, Any]) -> tuple[Image.Image, Image.Image]:
	params = family["render_parameters"]
	size = tuple(family["output"]["dimensions_px"])
	base = Image.new("RGB", size, parse_color(params["base"]))
	base = apply_unique_shapes(base, size, params["field_washes"], params["field_softness_px"])
	overlay = Image.new("RGBA", size, (0, 0, 0, 0))
	draw = ImageDraw.Draw(overlay)
	w, h = size
	inset = float(params["border_margin"]) * min(size)
	band_width = max(1, int(round(min(size) * 0.01)))
	for band in params["border_bands"]:
		width = max(1, int(round(min(size) * float(band["width"]))))
		box = (int(round(inset)), int(round(inset)), int(round(w - inset)), int(round(h - inset)))
		draw.rectangle(box, outline=(*parse_color(band["color"]), 255), width=max(width, band_width))
		inset += width + max(1, band_width // 2)
	for motif in params["motifs"]:
		draw_rug_motif(draw, motif, size, (*parse_color(motif["color"]), 255))
	base = Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")
	smk = render_smk(size, params, False)
	detail = params["smk"].get("detail_value", 0.55)
	detail_alpha = opacity_byte(params["smk"].get("detail_opacity", 180))
	detail_overlay = Image.new("RGBA", size, (0, 0, 0, 0))
	detail_draw = ImageDraw.Draw(detail_overlay)
	inset = float(params["border_margin"]) * min(size)
	for band in params["border_bands"]:
		width = max(1, int(round(min(size) * float(band["width"]))))
		box = (int(round(inset)), int(round(inset)), int(round(w - inset)), int(round(h - inset)))
		value = int(round(float(detail) * 255.0))
		detail_draw.rectangle(box, outline=(value, value, value, detail_alpha), width=max(width, band_width))
		inset += width + max(1, band_width // 2)
	for motif in params["motifs"]:
		value = int(round(float(detail) * 255.0))
		draw_rug_motif(detail_draw, motif, size, (value, value, value, detail_alpha))
	r, g, b = smk.split()
	r = Image.alpha_composite(r.convert("RGBA"), detail_overlay).convert("L")
	smk = Image.merge("RGB", (r, g, b))
	return base, smk


def render_family(family: dict[str, Any]) -> tuple[Image.Image, Image.Image] | None:
	generator = family["generator"]
	if generator == "cool_plaster":
		return render_cool_plaster(family)
	if generator == "floor_mass":
		return render_floor_mass(family)
	if generator == "wood":
		return render_wood(family)
	if generator == "rug":
		return render_rug(family)
	return None


def save_rgb(image: Image.Image, path: Path) -> None:
	path.parent.mkdir(parents=True, exist_ok=True)
	image.convert("RGB").save(path, format="PNG", optimize=True)


def sha256(path: Path) -> str:
	hasher = hashlib.sha256()
	with path.open("rb") as stream:
		for chunk in iter(lambda: stream.read(1024 * 1024), b""):
			hasher.update(chunk)
	return hasher.hexdigest()


def file_record(path: Path, role: str) -> dict[str, Any]:
	with Image.open(path) as image:
		return {
			"role": role,
			"path": relative_path(path),
			"dimensions_px": list(image.size),
			"mode": image.mode,
			"channels": "RGB" if image.mode == "RGB" else image.mode,
			"alpha": "A" in image.getbands(),
			"bytes": path.stat().st_size,
			"sha256": sha256(path),
			"exists": True,
		}


def validate_source_contract(config: dict[str, Any]) -> dict[str, Any]:
	checks: list[dict[str, Any]] = []
	for key, relative in config["source_contract"].items():
		path = repo_path(relative)
		checks.append({"id": key, "path": relative, "exists": path.is_file()})
	materials = read_json(repo_path(config["source_contract"]["iteration02_materials"]))
	for family in config["families"]:
		if not family.get("source_bc"):
			continue
		filename = Path(family["source_bc"]).name
		missing = []
		mismatched = []
		for material_id in family.get("source_material_ids", []):
			entry = materials.get(material_id)
			if entry is None:
				missing.append(material_id)
			elif entry.get("BC") != filename:
				mismatched.append({"material": material_id, "actual": entry.get("BC"), "expected": filename})
		checks.append({"id": f"materials:{family['id']}", "missing": missing, "mismatched": mismatched, "ok": not missing and not mismatched})
	cloth = read_json(repo_path(config["source_contract"]["iteration02_cloth_atlas_manifest"]))
	for mapping in next(f for f in config["families"] if f["id"] == "ClothAtlas")["root_mappings"]:
		match = next((item for item in cloth if item.get("root") == mapping["root"]), None)
		ok = bool(match and match.get("BC") == Path(mapping["atlas_bc"]).name and match.get("resolution") == mapping["atlas_resolution_px"][0])
		checks.append({"id": f"cloth:{mapping['root']}", "ok": ok})
	return {"ok": all(item.get("exists", True) and item.get("ok", True) for item in checks), "checks": checks}


def validate_candidate(path: Path, expected_size: tuple[int, int], seamless: bool) -> dict[str, Any]:
	result = {"path": relative_path(path), "exists": path.is_file(), "expected_dimensions_px": list(expected_size)}
	if not path.is_file():
		result["ok"] = False
		return result
	with Image.open(path) as image:
		result.update({"dimensions_px": list(image.size), "mode": image.mode, "channels": image.getbands(), "alpha": "A" in image.getbands()})
		result["ok"] = image.size == expected_size and image.mode == "RGB" and "A" not in image.getbands()
		if seamless and image.size == expected_size:
			left = ImageOps.grayscale(image.crop((0, 0, 1, image.height))).resize((1, image.height))
			right = ImageOps.grayscale(image.crop((image.width - 1, 0, image.width, image.height))).resize((1, image.height))
			top = ImageOps.grayscale(image.crop((0, 0, image.width, 1))).resize((image.width, 1))
			bottom = ImageOps.grayscale(image.crop((0, image.height - 1, image.width, image.height))).resize((image.width, 1))
			result["edge_mean_abs_diff"] = {"horizontal": ImageOps.invert(ImageOps.grayscale(ImageChops.difference(left, right))).getextrema()[1], "vertical": ImageOps.invert(ImageOps.grayscale(ImageChops.difference(top, bottom))).getextrema()[1]}
	return result


def preview_pair(family: dict[str, Any], bc: Image.Image, smk: Image.Image, output_dir: Path) -> Path:
	card_size = (640, 430)
	canvas = Image.new("RGB", (card_size[0] * 2, card_size[1] + 46), (245, 240, 229))
	font = None
	for index, (image, label) in enumerate(((bc, "BC candidate"), (smk, "SMK debug"))):
		preview = ImageOps.contain(image.convert("RGB"), card_size)
		x = index * card_size[0] + (card_size[0] - preview.width) // 2
		y = 42 + (card_size[1] - preview.height) // 2
		canvas.paste(preview, (x, y))
		draw = ImageDraw.Draw(canvas)
		draw.text((index * card_size[0] + 14, 13), label, fill=(40, 35, 31), font=font)
	path = output_dir / f"{family['id']}_CandidatePreview.png"
	canvas.save(path, format="PNG", optimize=True)
	return path


def make_contact_sheet(previews: list[Path], output_dir: Path) -> Path | None:
	if not previews:
		return None
	thumb_w, thumb_h = 640, 245
	columns = 2
	rows = math.ceil(len(previews) / columns)
	sheet = Image.new("RGB", (columns * thumb_w, rows * thumb_h), (236, 230, 219))
	for index, path in enumerate(previews):
		with Image.open(path) as image:
			thumb = ImageOps.contain(image.convert("RGB"), (thumb_w - 18, thumb_h - 18))
		x = (index % columns) * thumb_w + (thumb_w - thumb.width) // 2
		y = (index // columns) * thumb_h + (thumb_h - thumb.height) // 2
		sheet.paste(thumb, (x, y))
	path = output_dir / "P2_WatercolorSamples_ContactSheet.png"
	sheet.save(path, format="PNG", optimize=True)
	return path


def build_manifest(config: dict[str, Any], source_check: dict[str, Any], generated: list[str], previews: list[Path], run_family: str, config_path: Path) -> dict[str, Any]:
	root = repo_path(config["output"]["root"])
	entries = []
	for family in config["families"]:
		entry = {"id": family["id"], "generator": family["generator"], "config_status": family["status_if_generated"], "source_material_ids": family.get("source_material_ids", []), "ue_binding": family["ue_binding"], "physical_scale": family.get("physical_scale"), "files": [], "art_review": "pending_manual_visual_review", "ue_import": "not_run"}
		if family["id"] == "ClothAtlas":
			entry.update({"status": "deferred_atlas_bake", "files": [], "art_review": "blocked_until_island_aware_bake"})
		elif family["id"] == "Glass":
			entry.update({"status": "config_only_no_texture", "files": [], "art_review": "recipe_only"})
		else:
			bc_path = root / family["output"]["bc"]
			smk_path = root / family["output"]["smk"]
			if bc_path.is_file():
				entry["files"].append(file_record(bc_path, "BC"))
			if smk_path.is_file():
				entry["files"].append(file_record(smk_path, "SMK"))
			entry["status"] = "candidate_source_generated" if family["id"] in generated else ("existing_candidate" if entry["files"] else "not_generated")
		entry["preview"] = relative_path(next((p for p in previews if p.name.startswith(family["id"])), previews[0])) if any(p.name.startswith(family["id"]) for p in previews) else None
		entries.append(entry)
	manifest = {
		"schema_version": "1.0",
		"purpose": "P2 independent watercolor source candidates; manual art review is required before UE import.",
		"generated_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
		"run_family": run_family,
		"generated_families": generated,
		"camera_contract": config["camera_contract"],
		"source_references": config["source_contract"],
		"source_contract_check": source_check,
		"config_path": relative_path(config_path),
		"config_sha256": sha256(config_path),
		"script_sha256": sha256(Path(__file__)),
		"no_geometry_change": True,
		"no_ue_import": True,
		"families": entries,
		"previews": [relative_path(path) for path in previews],
	}
	return manifest


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
	parser.add_argument("--family", default="all", help="all or one family id")
	parser.add_argument("--validate-only", action="store_true")
	return parser.parse_args()


def main() -> int:
	args = parse_args()
	config_path = args.config if args.config.is_absolute() else ROOT / args.config
	config = read_json(config_path)
	source_check = validate_source_contract(config)
	if args.validate_only:
		print(json.dumps(source_check, ensure_ascii=True, indent=2))
		return 0 if source_check["ok"] else 2
	families = {family["id"]: family for family in config["families"]}
	if args.family != "all" and args.family not in families:
		raise SystemExit(f"Unknown family {args.family!r}; choose all or {', '.join(families)}")
	selected = set(families) if args.family == "all" else {args.family}
	root = repo_path(config["output"]["root"])
	preview_dir = root / config["output"]["preview_directory"]
	generated = []
	previews = []
	for family in config["families"]:
		if family["id"] not in selected or family["generator"] not in GENERATORS:
			continue
		bc, smk = render_family(family) or (None, None)
		if bc is None or smk is None:
			continue
		save_rgb(bc, root / family["output"]["bc"])
		save_rgb(smk, root / family["output"]["smk"])
		previews.append(preview_pair(family, bc, smk, preview_dir))
		generated.append(family["id"])
	contact = make_contact_sheet(previews, preview_dir)
	if contact:
		previews.append(contact)
	manifest = build_manifest(config, source_check, generated, previews, args.family, config_path)
	write_json(root / config["output"]["manifest"], manifest)
	for family in config["families"]:
		if family["id"] not in generated or family["generator"] not in GENERATORS:
			continue
		for role in ("bc", "smk"):
			path = root / family["output"][role]
			check = validate_candidate(path, tuple(family["output"]["dimensions_px"]), bool(family["output"].get("seamless", False)))
			if not check["ok"]:
				raise SystemExit(f"Candidate validation failed: {json.dumps(check, ensure_ascii=True)}")
	print(json.dumps({"generated": generated, "manifest": relative_path(root / config["output"]["manifest"]), "source_contract_ok": source_check["ok"], "previews": [relative_path(path) for path in previews]}, ensure_ascii=True, indent=2))
	return 0


if __name__ == "__main__":
	main()
