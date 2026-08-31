"""Generate deterministic M1 foliage atlas textures.

The atlas deliberately favors continuous cluster silhouettes and low-frequency
color masses. Transparent texels keep dilated edge color so generated mips do
not inherit a black fringe.
"""

from pathlib import Path
import math

from PIL import Image, ImageChops, ImageDraw, ImageFilter


SIZE = 1024
OUTPUT_DIR = Path(__file__).resolve().parent / "Generated" / "M1Foliage"


def _cluster_mask(rect: tuple[int, int, int, int], variant: int) -> Image.Image:
	mask = Image.new("L", (SIZE, SIZE), 0)
	draw = ImageDraw.Draw(mask)
	x0, y0, x1, y1 = rect
	cx = (x0 + x1) // 2
	cy = (y0 + y1) // 2
	rx = (x1 - x0) * 0.38
	ry = (y1 - y0) * 0.34
	for index in range(9):
		angle = (index * 2.399963 + variant * 0.41)
		radial = 0.16 + 0.16 * (index % 3)
		x = cx + math.cos(angle) * rx * radial
		y = cy + math.sin(angle) * ry * radial
		w = rx * (0.56 + 0.07 * ((index + variant) % 3))
		h = ry * (0.58 + 0.06 * ((index + 1) % 4))
		draw.ellipse((x - w, y - h, x + w, y + h), fill=255)
	# Only two large holes: enough negative space without turning the card into
	# dozens of isolated leaf islands.
	hole = int((x1 - x0) * 0.055)
	draw.ellipse((cx - rx * 0.35 - hole, cy - hole, cx - rx * 0.35 + hole, cy + hole), fill=0)
	draw.ellipse((cx + rx * 0.28 - hole, cy + ry * 0.08 - hole, cx + rx * 0.28 + hole, cy + ry * 0.08 + hole), fill=0)
	return mask.filter(ImageFilter.GaussianBlur(1.2))


def _grass_mask(rect: tuple[int, int, int, int], hero: bool) -> Image.Image:
	mask = Image.new("L", (SIZE, SIZE), 0)
	draw = ImageDraw.Draw(mask)
	x0, y0, x1, y1 = rect
	base_y = y1 - 34
	count = 11 if hero else 8
	for index in range(count):
		fraction = (index + 0.5) / count
		root_x = x0 + 42 + fraction * (x1 - x0 - 84)
		tip_x = root_x + math.sin(index * 1.73) * (42 if hero else 30)
		tip_y = y0 + (45 if hero else 95) + (index % 4) * 18
		width = 22 if hero else 18
		points = [
			(root_x - width, base_y),
			(root_x - width * 0.45, (base_y + tip_y) * 0.55),
			(tip_x, tip_y),
			(root_x + width * 0.45, (base_y + tip_y) * 0.55),
			(root_x + width, base_y),
		]
		draw.polygon(points, fill=255)
	draw.rounded_rectangle((x0 + 34, base_y - 26, x1 - 34, base_y + 12), radius=18, fill=255)
	return mask.filter(ImageFilter.GaussianBlur(0.8))


def _paint_low_frequency(base: Image.Image, rect: tuple[int, int, int, int], palette: tuple[tuple[int, int, int], ...], phase: float) -> None:
	pixels = base.load()
	x0, y0, x1, y1 = rect
	for y in range(y0, y1):
		for x in range(x0, x1):
			u = (x - x0) / max(1, x1 - x0)
			v = (y - y0) / max(1, y1 - y0)
			band = 0.5 + 0.28 * math.sin(u * math.pi * 2.0 + phase) + 0.22 * math.cos(v * math.pi * 2.0 - phase)
			index = min(len(palette) - 1, max(0, int(band * len(palette))))
			pixels[x, y] = (*palette[index], 255)


def generate() -> None:
	OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
	base = Image.new("RGBA", (SIZE, SIZE), (67, 101, 57, 255))
	alpha = Image.new("L", (SIZE, SIZE), 0)

	regions = {
		"tree_a": (24, 24, 500, 500),
		"tree_b": (524, 24, 1000, 500),
		"grass": (24, 524, 500, 1000),
		"hero": (524, 524, 1000, 1000),
	}
	_paint_low_frequency(base, regions["tree_a"], ((55, 93, 54), (78, 123, 61), (111, 147, 70)), 0.2)
	_paint_low_frequency(base, regions["tree_b"], ((48, 83, 61), (67, 111, 71), (98, 139, 73)), 1.1)
	_paint_low_frequency(base, regions["grass"], ((47, 83, 48), (69, 112, 55), (103, 143, 66)), 0.7)
	_paint_low_frequency(base, regions["hero"], ((51, 84, 53), (83, 124, 60), (125, 154, 70)), 1.7)

	for mask in (
		_cluster_mask(regions["tree_a"], 0),
		_cluster_mask(regions["tree_b"], 1),
		_grass_mask(regions["grass"], False),
		_grass_mask(regions["hero"], True),
	):
		alpha = ImageChops.lighter(alpha, mask)
	base.putalpha(alpha)
	base.save(OUTPUT_DIR / "T_LR_M1_Foliage_BC.png")

	smk = Image.new("RGBA", (SIZE, SIZE), (90, 0, 128, 0))
	smk_pixels = smk.load()
	for y in range(SIZE):
		for x in range(SIZE):
			macro = int(128 + 22 * math.sin(x / 173.0) * math.cos(y / 211.0))
			smk_pixels[x, y] = (90, 0, max(96, min(160, macro)), 0)
	smk.save(OUTPUT_DIR / "T_LR_M1_Foliage_SMK.png")

	normal = Image.new("RGB", (SIZE, SIZE), (128, 128, 255))
	normal_pixels = normal.load()
	for y in range(SIZE):
		for x in range(SIZE):
			nx = 8.0 * math.sin(x / 91.0) * math.cos(y / 137.0)
			ny = 8.0 * math.cos(x / 127.0) * math.sin(y / 83.0)
			normal_pixels[x, y] = (int(128 + nx), int(128 + ny), 255)
	normal.save(OUTPUT_DIR / "T_LR_M1_Foliage_NRM.png")


if __name__ == "__main__":
	generate()
