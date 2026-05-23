import torch
import random
import os
import sys

sys.path.append(os.path.dirname(__file__))
import macro_packer 

class GeometricLegalizer:
	def place(self, benchmark):
		placement = benchmark.macro_positions.clone()
		sizes = benchmark.macro_sizes
		movable = benchmark.get_movable_mask() & benchmark.get_hard_macro_mask()
		movable_indices = torch.where(movable)[0].tolist()

		SCALE = 1000.0  
		canvas_w = int(benchmark.canvas_width * SCALE)
		canvas_h = int(benchmark.canvas_height * SCALE)

		macro_targets = {}
		for idx in movable_indices:
			macro_targets[idx] = {
				"x": placement[idx, 0].item() * SCALE,
				"y": placement[idx, 1].item() * SCALE,
				"w": sizes[idx, 0].item(),
				"h": sizes[idx, 1].item()
			}

		# C++ LEGALIZATION
		HALO = int(0.10 * SCALE) 
		shapes = []
		for idx in movable_indices:
			w = int(macro_targets[idx]["w"] * SCALE) + HALO
			h = int(macro_targets[idx]["h"] * SCALE) + HALO
			shape = macro_packer.Shape(idx, 0, 0, w, h)
			shape.set_target(macro_targets[idx]["x"] - (w/2), macro_targets[idx]["y"] - (h/2))
			shapes.append(shape)
			
		print(f"Legalizing...")
		result = macro_packer.solve(canvas_w, canvas_h, shapes, macro_packer.Heuristic.DescendingArea, False)

		for shape in result.rectangles:
			idx = shape.id()
			placement[idx, 0] = (shape.x() + (sizes[idx, 0].item() * SCALE / 2.0)) / SCALE
			placement[idx, 1] = (shape.y() + (sizes[idx, 1].item() * SCALE / 2.0)) / SCALE

		return placement