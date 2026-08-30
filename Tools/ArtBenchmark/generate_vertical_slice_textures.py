"""Generate deterministic source textures for the Normal rendering vertical slice.

Base color is deliberately low-frequency and illustrative. SMK, detail, and normal
maps are derived from explicit procedural rules so their channel semantics remain
auditable and reproducible.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


SIZE = 512
OUTPUT_DIR = Path(__file__).resolve().parent / "Generated"


def save_rgb(name: str, rgb: np.ndarray) -> None:
	image = np.clip(rgb * 255.0 + 0.5, 0, 255).astype(np.uint8)
	Image.fromarray(image, "RGB").save(OUTPUT_DIR / f"{name}.png")


def save_rgba(name: str, rgba: np.ndarray) -> None:
	image = np.clip(rgba * 255.0 + 0.5, 0, 255).astype(np.uint8)
	Image.fromarray(image, "RGBA").save(OUTPUT_DIR / f"{name}.png")


def blur(field: np.ndarray, radius: float) -> np.ndarray:
	image = Image.fromarray(np.clip(field * 255.0 + 0.5, 0, 255).astype(np.uint8), "L")
	return np.asarray(image.filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32) / 255.0


def normalize01(field: np.ndarray) -> np.ndarray:
	minimum = float(field.min())
	maximum = float(field.max())
	return (field - minimum) / max(maximum - minimum, 1.0e-6)


def normal_from_height(height: np.ndarray, strength: float) -> np.ndarray:
	gradient_y, gradient_x = np.gradient(height)
	normal = np.dstack((-gradient_x * strength, -gradient_y * strength, np.ones_like(height)))
	normal /= np.maximum(np.linalg.norm(normal, axis=2, keepdims=True), 1.0e-6)
	return normal * 0.5 + 0.5


def coordinates() -> tuple[np.ndarray, np.ndarray]:
	axis = np.linspace(0.0, 1.0, SIZE, endpoint=False, dtype=np.float32)
	return np.meshgrid(axis, axis)


def make_material(kind: str, seed: int) -> None:
	x, y = coordinates()
	rng = np.random.default_rng(seed)
	noise = rng.random((SIZE, SIZE), dtype=np.float32)
	low = normalize01(blur(noise, 42.0))

	if kind == "Wood":
		direction = np.sin((x * 10.0 + 0.12 * np.sin(y * 6.0)) * np.pi * 2.0)
		height = 0.5 + direction * 0.10 + (low - 0.5) * 0.18
		base = np.dstack((0.34 + height * 0.24, 0.16 + height * 0.16, 0.08 + height * 0.10))
		detail_allowed = np.full_like(height, 0.82)
		structure = ((np.mod(x * 5.0, 1.0) < 0.018) | (np.mod(y * 2.0, 1.0) < 0.012)).astype(np.float32)
		normal_strength = 5.0
	elif kind == "Plaster":
		height = 0.5 + (blur(noise, 18.0) - 0.5) * 0.20
		base = np.dstack((0.62 + low * 0.12, 0.59 + low * 0.10, 0.56 + low * 0.12))
		detail_allowed = np.full_like(height, 0.28)
		structure = np.zeros_like(height)
		normal_strength = 2.0
	elif kind == "Wallpaper":
		motif = 0.5 + 0.5 * np.cos(x * np.pi * 12.0) * np.cos(y * np.pi * 16.0)
		height = 0.5 + (motif - 0.5) * 0.10
		base = np.dstack((0.46 + motif * 0.18, 0.43 + motif * 0.14, 0.54 + motif * 0.16))
		detail_allowed = np.full_like(height, 0.62)
		structure = np.zeros_like(height)
		normal_strength = 2.5
	elif kind == "Metal":
		brushed = 0.5 + 0.5 * np.sin(y * np.pi * 90.0)
		height = 0.5 + (blur(brushed, 1.5) - 0.5) * 0.08
		base = np.dstack((0.32 + low * 0.14, 0.34 + low * 0.13, 0.36 + low * 0.14))
		detail_allowed = np.full_like(height, 0.34)
		structure = np.zeros_like(height)
		normal_strength = 1.8
	elif kind == "Cloth":
		weave = 0.5 + 0.25 * np.sin(x * np.pi * 48.0) + 0.25 * np.sin(y * np.pi * 48.0)
		height = 0.5 + (weave - 0.5) * 0.14
		base = np.dstack((0.24 + low * 0.13, 0.30 + low * 0.13, 0.36 + low * 0.16))
		detail_allowed = np.full_like(height, 0.58)
		structure = np.zeros_like(height)
		normal_strength = 3.0
	else:
		raise ValueError(kind)

	# High-frequency separation is recentered so 0.5 is exactly neutral.
	high = height - blur(height, 14.0)
	high /= max(float(np.max(np.abs(high))), 1.0e-6)
	detail = np.clip(0.5 + high * 0.34, 0.0, 1.0)
	macro = np.clip(0.5 + (low - 0.5) * 0.72, 0.15, 0.85)
	smk = np.dstack((detail_allowed, structure, macro, np.zeros_like(height)))

	save_rgb(f"T_LR_Benchmark_{kind}_BC", base)
	save_rgba(f"T_LR_Benchmark_{kind}_SMK", smk)
	save_rgb(f"T_LR_Benchmark_{kind}_Detail", np.dstack((detail, detail, detail)))
	save_rgb(f"T_LR_Benchmark_{kind}_NRM", normal_from_height(height, normal_strength))


def make_shared_wash() -> None:
	x, y = coordinates()
	field = 0.5 + 0.18 * np.sin((x * 2.0 + y * 1.25) * np.pi * 2.0)
	field += 0.09 * np.sin((x * 5.0 - y * 3.0) * np.pi * 2.0)
	field = np.clip(blur(field, 7.0), 0.2, 0.8)
	wash = np.dstack((field, np.clip(field * 0.96 + 0.02, 0.0, 1.0), np.clip(field * 1.04 - 0.02, 0.0, 1.0)))
	save_rgb("T_LR_Shared_PainterlyWash_A", wash)


def main() -> None:
	OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
	for index, kind in enumerate(("Wood", "Plaster", "Wallpaper", "Metal", "Cloth"), start=1):
		make_material(kind, 1729 + index * 97)
	make_shared_wash()


if __name__ == "__main__":
	main()
