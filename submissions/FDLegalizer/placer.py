import torch
import random
import os
import sys

sys.path.append(os.path.dirname(__file__))
import macro_packer 

class PhysicsLegalizerPlacer:
	def place(self, benchmark):
		placement = benchmark.macro_positions.clone()
		sizes = benchmark.macro_sizes
		movable = benchmark.get_movable_mask() & benchmark.get_hard_macro_mask()
		movable_indices = torch.where(movable)[0].tolist()

		placement = self.optimize_soft_macros(benchmark, placement)

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

		macro_to_nets = {idx: [] for idx in movable_indices}
		if hasattr(benchmark, 'nets'):
			for net in benchmark.nets:
				for pin in net.pins:
					if pin.is_macro and pin.macro_id in macro_to_nets:
						macro_to_nets[pin.macro_id].append(net)

		ideal_pos = {idx: {"x": placement[idx, 0].item() * SCALE, "y": placement[idx, 1].item() * SCALE} for idx in movable_indices}

		ITERATIONS = 400
		LEARNING_RATE = 0.2
		REPULSION_WEIGHT = 0.005
		
		print(f"[my_placer] Simulating forces (Iterations: {ITERATIONS})...")
		for step in range(ITERATIONS):
			force_x = {idx: 0.0 for idx in movable_indices}
			force_y = {idx: 0.0 for idx in movable_indices}

			CENTER_X = canvas_w / 2.0
			CENTER_Y = canvas_h / 2.0
			
			if step % 10 == 0:
				for idx in movable_indices:
					cx, cy, count = 0.0, 0.0, 0
					for net in macro_to_nets[idx]:
						for p in net.pins:
							if p.is_macro:
								if p.macro_id != idx:
									if p.macro_id in macro_targets:
										cx += macro_targets[p.macro_id]["x"]
										cy += macro_targets[p.macro_id]["y"]
									else:
										cx += placement[p.macro_id, 0].item() * SCALE
										cy += placement[p.macro_id, 1].item() * SCALE
									count += 1
							else:
								cx += p.x * SCALE
								cy += p.y * SCALE
								count += 1
					if count > 0:
						ideal_pos[idx]["x"] = cx / count
						ideal_pos[idx]["y"] = cy / count

			for idx in movable_indices:
				ideal_x = ideal_pos[idx]["x"]
				ideal_y = ideal_pos[idx]["y"]
				
				force_x[idx] += (ideal_x - macro_targets[idx]["x"]) * 0.08
				force_y[idx] += (ideal_y - macro_targets[idx]["y"]) * 0.08
				
				force_x[idx] += (CENTER_X - macro_targets[idx]["x"]) * 0.005
				force_y[idx] += (CENTER_Y - macro_targets[idx]["y"]) * 0.005

			for i, idx1 in enumerate(movable_indices):
				for j, idx2 in enumerate(movable_indices):
					if i >= j: continue
					m1, m2 = macro_targets[idx1], macro_targets[idx2]
					dx = m1["x"] - m2["x"]
					dy = m1["y"] - m2["y"]
					dist_sq = dx*dx + dy*dy + 1e-6
					min_dist = (m1["w"] + m2["w"]) * SCALE * 0.7
					
					if dist_sq < min_dist**2:
						dist = dist_sq**0.5
						repel = REPULSION_WEIGHT * (min_dist - dist) / dist
						force_x[idx1] += dx * repel
						force_y[idx1] += dy * repel
						force_x[idx2] -= dx * repel
						force_y[idx2] -= dy * repel

			for idx in movable_indices:
				macro_targets[idx]["x"] += force_x[idx] * LEARNING_RATE
				macro_targets[idx]["y"] += force_y[idx] * LEARNING_RATE
				macro_targets[idx]["x"] = max(0, min(canvas_w, macro_targets[idx]["x"]))
				macro_targets[idx]["y"] = max(0, min(canvas_h, macro_targets[idx]["y"]))

			REPULSION_WEIGHT = min(REPULSION_WEIGHT * 1.015, 0.1)

		# C++ LEGALIZATION
		HALO = int(0.10 * SCALE) 
		shapes = []
		for idx in movable_indices:
			w = int(macro_targets[idx]["w"] * SCALE) + HALO
			h = int(macro_targets[idx]["h"] * SCALE) + HALO
			shape = macro_packer.Shape(idx, 0, 0, w, h)
			shape.set_target(macro_targets[idx]["x"] - (w/2), macro_targets[idx]["y"] - (h/2))
			shapes.append(shape)
			
		print(f"[my_placer] Legalizing...")
		result = macro_packer.solve(canvas_w, canvas_h, shapes, macro_packer.Heuristic.DescendingArea, False)

		for shape in result.rectangles:
			idx = shape.id()
			placement[idx, 0] = (shape.x() + (sizes[idx, 0].item() * SCALE / 2.0)) / SCALE
			placement[idx, 1] = (shape.y() + (sizes[idx, 1].item() * SCALE / 2.0)) / SCALE

		placement = self.greedy_refine(benchmark, placement, movable_indices)
		placement = self.optimize_soft_macros(benchmark, placement)
		
		return placement

	def get_local_hpwl(self, benchmark, placement, net_list):
		"""Calculates total HPWL for a specific subset of nets."""
		hpwl = 0.0
		for net in net_list:
			min_x, max_x = 1e12, -1e12
			min_y, max_y = 1e12, -1e12
			for pin in net.pins:
				if pin.is_macro:
					px = placement[pin.macro_id, 0].item() + pin.x_offset
					py = placement[pin.macro_id, 1].item() + pin.y_offset
				else:
					px, py = pin.x, pin.y
				if px < min_x: min_x = px
				if px > max_x: max_x = px
				if py < min_y: min_y = py
				if py > max_y: max_y = py
			hpwl += (max_x - min_x) + (max_y - min_y)
		return hpwl
	
	def optimize_soft_macros(self, benchmark, placement):
		soft_mask = benchmark.get_soft_macro_mask() & benchmark.get_movable_mask()
		soft_indices = torch.where(soft_mask)[0].tolist()
		if not hasattr(benchmark, 'nets') or not soft_indices:
			return placement

		print("[my_placer] Optimizing soft macros (Iterative Average)...")
		for _ in range(15):
			targets_x = {idx: [] for idx in soft_indices}
			targets_y = {idx: [] for idx in soft_indices}

			for net in benchmark.nets:
				xs, ys = [], []
				for pin in net.pins:
					if pin.is_macro:
						xs.append(placement[pin.macro_id, 0].item() + pin.x_offset)
						ys.append(placement[pin.macro_id, 1].item() + pin.y_offset)
					else:
						xs.append(pin.x)
						ys.append(pin.y)
				if not xs: continue
				cg_x, cg_y = sum(xs)/len(xs), sum(ys)/len(ys)

				for pin in net.pins:
					if pin.is_macro and pin.macro_id in soft_indices:
						targets_x[pin.macro_id].append(cg_x - pin.x_offset)
						targets_y[pin.macro_id].append(cg_y - pin.y_offset)

			for idx in soft_indices:
				if targets_x[idx]:
					avg_x = sum(targets_x[idx]) / len(targets_x[idx])
					avg_y = sum(targets_y[idx]) / len(targets_y[idx])
					
					jitter_x = (random.random() - 0.5) * 4.0 
					jitter_y = (random.random() - 0.5) * 4.0
					
					placement[idx, 0] = avg_x + jitter_x
					placement[idx, 1] = avg_y + jitter_y

			placement[soft_indices, 0] = torch.clamp(placement[soft_indices, 0], 0, benchmark.canvas_width)
			placement[soft_indices, 1] = torch.clamp(placement[soft_indices, 1], 0, benchmark.canvas_height)

		return placement

	def greedy_refine(self, benchmark, placement, movable_indices):
		if not hasattr(benchmark, 'nets'): return placement
		
		print("[my_placer] Starting local HPWL refinement...")
		
		macro_to_nets = {idx: [] for idx in movable_indices}
		for net in benchmark.nets:
			for pin in net.pins:
				if pin.is_macro and pin.macro_id in macro_to_nets:
					macro_to_nets[pin.macro_id].append(net)

		for _ in range(2000):
			idx1, idx2 = random.sample(movable_indices, 2)
			if not torch.allclose(benchmark.macro_sizes[idx1], benchmark.macro_sizes[idx2], atol=1e-3):
				continue
			
			affected_nets = list(set(macro_to_nets[idx1] + macro_to_nets[idx2]))
			
			old_hpwl = self.get_local_hpwl(benchmark, placement, affected_nets)
			
			pos1, pos2 = placement[idx1].clone(), placement[idx2].clone()
			placement[idx1], placement[idx2] = pos2, pos1
			
			new_hpwl = self.get_local_hpwl(benchmark, placement, affected_nets)
			
			if new_hpwl >= old_hpwl:
				placement[idx1], placement[idx2] = pos1, pos2
				
		return placement