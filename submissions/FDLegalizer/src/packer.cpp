/**==============================================
 * ?                    ABOUT
 * @author      : Romain BESSON
 * @description : Main packer algorithm
 *=============================================**/

#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include "packer.h"

#define CHECK_VALID true

constexpr uint32_t INT_INFINITY = 1000000000;

// Get Area
uint32_t Shape::area() const
{
	if (!this->area_.has_value())
	{
		this->area_ = w_ * h_;
	}
	return *this->area_;
}

// Compare two SHAPES based on basic properties (x,y,w,h)
bool Shape::operator==(const Shape &shape) const
{
	return (
		this->x_ == shape.x_ &&
		this->y_ == shape.y_ &&
		this->w_ == shape.w_ &&
		this->h_ == shape.h_);
}

// Check if a shape fits in another using
// a comparison of dimensions (theoretical)
bool Shape::fits_in(const Shape &shape) const
{
	return (
		this->w_ <= shape.w_ &&
		this->h_ <= shape.h_);
};

// Check if a shape fits in another using
// a comparison of positions (applied)
bool Shape::is_in(const Shape &shape) const
{
	return (
		this->x_ >= shape.x_ && // TL corner of 'this' shape is inside 'shape'
		this->y_ >= shape.y_ && //

		this->x2() <= shape.x2() && // BR corner of 'this' shape is inside 'shape'
		this->y2() <= shape.y2()	//
	);
}

// Detects overlap between two intervals A and B
bool overlaps(uint32_t Amin, uint32_t Amax, uint32_t Bmin, uint32_t Bmax)
{
	return Amin < Bmax && Amax > Bmin;
}

// Collision detection between 2 shapes
// (Sharing a common border is not considered an intersect)
bool Shape::intersects(const Shape &shape) const
{
	return (
		overlaps(this->x_, this->x2(), shape.x_, shape.x2()) && // Two shapes only overlap
		overlaps(this->y_, this->y2(), shape.y_, shape.y2())	// if they overlap in all dimensions
	);
}

bool Shape::is_covered(const std::vector<Shape> &shapes) const
{
	return std::any_of(shapes.begin(), shapes.end(), [this](Shape shape)
					   { return this->is_in(shape); });
}

// Sorting Strategy Heuristics
bool descending_area(const Shape &a, const Shape &b)
{
	return std::make_tuple(a.area(), a.w(), a.h(), -a.id()) >
		   std::make_tuple(b.area(), b.w(), b.h(), -b.id());
}
bool descending_area_2(const Shape &a, const Shape &b)
{
	return std::make_tuple(a.area() + a.h(), a.w(), a.h(), -a.id()) >
		   std::make_tuple(b.area() + b.h(), b.w(), b.h(), -b.id());
}
bool descending_width(const Shape &a, const Shape &b)
{
	return std::make_tuple(a.w(), a.h(), a.area(), -a.id()) >
		   std::make_tuple(b.w(), b.h(), b.area(), -b.id());
}
bool descending_height(const Shape &a, const Shape &b)
{
	return std::make_tuple(a.h(), a.w(), a.area(), -a.id()) >
		   std::make_tuple(b.h(), b.w(), b.area(), -b.id());
}

// Creates/Cuts Hole into new Holes based on a Shape
void cut_hole(const Shape &rectangle, const Shape &hole, std::vector<Shape> &holes, uint32_t &next_hole_id)
{
	// Create new holes from a Placement / Overlapping
	// ⬛ => Hole
	// ⬜ => Rectangle
	// So we enumerate all the 16 cases where the rectangle can clip inside the Hole

	uint32_t rectangle_x2 = rectangle.x2();
	uint32_t rectangle_y2 = rectangle.y2();
	uint32_t hole_x2 = hole.x2();
	uint32_t hole_y2 = hole.y2();

	std::vector<Shape> new_holes{};

	// ⬜⬜⬜
	// ⬜⬜⬜
	// ⬜⬜⬜
	// [CASE 1]: Perfect fit!
	if (
		rectangle == hole)
	{
	} // Only case where we add 0 Holes
	//

	// ⬜⬜⬜
	// ⬛⬛⬛
	// ⬛⬛⬛
	// [CASE 2]: Top Bar Horizontal
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND
		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge is right to the hole AND

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge passes through the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
	}
	//

	// ⬛⬛⬜
	// ⬛⬛⬜
	// ⬛⬛⬜
	// [CASE 3]: Right Bar Vertical
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge is right to the hole AND

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
	}
	//

	// ⬛⬛⬛
	// ⬛⬛⬛
	// ⬜⬜⬜
	// [CASE 4]: Bottom Bar Horizontal
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge is right to the hole AND

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
	}
	//

	// ⬜⬛⬛
	// ⬜⬛⬛
	// ⬜⬛⬛
	// [CASE 5]: Left Bar Vertical
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}
	//

	// ⬜⬛⬛
	// ⬛⬛⬛
	// ⬛⬛⬛
	// [CASE 6]: Top Left Corner
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge passes through the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}
	//

	// ⬛⬛⬜
	// ⬛⬛⬛
	// ⬛⬛⬛
	// [CASE 7]: Top Right Corner
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge Crosses in the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge is right to the hole AND
		rectangle_x2 >= hole_x2 && //

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge passes through the Hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
	}
	//

	// ⬛⬛⬛
	// ⬛⬛⬛
	// ⬛⬛⬜
	// [CASE 8]: Bottom Right Corner
	else if (
		rectangle.x() < hole_x2 &&	// Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() > hole.x() && //

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge is right to the hole AND

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
	}

	// ⬛⬛⬛
	// ⬛⬛⬛
	// ⬜⬛⬛
	// [CASE 9]: Bottom Left Corner
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge passes through the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}

	// ⬛⬜⬛
	// ⬛⬜⬛
	// ⬛⬜⬛
	// [CASE 10]: Middle Vertical Bar
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}

	// ⬛⬛⬛
	// ⬜⬜⬜
	// ⬛⬛⬛
	// [CASE 11]: Middle Horizontal Bar
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge if right to the hole AND

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge is below the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
	}

	// ⬛⬜⬛
	// ⬛⬛⬛
	// ⬛⬛⬛
	// [CASE 12]: Top Rectangle Pop-Out
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() <= hole.y() && // Rectangle's Top Horizontal Edge is above the hole AND

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge pases through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge passes through the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}

	// ⬛⬛⬛
	// ⬛⬛⬜
	// ⬛⬛⬛
	// [CASE 13]: Right Rectangle Pop-Out
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 >= hole_x2 && // Rectangle's Right Vertical Edge is right to the hole AND

		rectangle_y2 > hole.y() && // Rectangle's Bottom Horizontal Edge is below the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
	}

	// ⬛⬛⬛
	// ⬛⬛⬛
	// ⬛⬜⬛
	// [CASE 14]: Bottom Rectangle Pop-Out
	else if (
		rectangle.x() > hole.x() && // Rectangle's Left Vertical Edge passes through the hole AND
		rectangle.x() < hole_x2 &&	//

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 > hole.x() && // Rectangle's Right Vertical Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 >= hole_y2 // Rectangle's Bottom Horizontal Edge is below the hole
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}

	// ⬛⬛⬛
	// ⬜⬛⬛
	// ⬛⬛⬛
	// [CASE 15]: Left Rectangle Pop-Out
	else if (
		rectangle.x() <= hole.x() && // Rectangle's Left Vertical Edge is left to the hole AND

		rectangle.y() > hole.y() && // Rectangle's Top Horizontal Edge passes through the hole AND
		rectangle.y() < hole_y2 &&	//

		rectangle_x2 > hole.x() && // Rectangle's Bottom Horizontal Edge passes through the hole AND
		rectangle_x2 < hole_x2 &&  //

		rectangle_y2 > hole.y() && // Rectangle's Right Vertical Edge passes through the hole
		rectangle_y2 < hole_y2	   //
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
	}

	// ⬜⬜⬜
	// ⬜⬛⬜
	// ⬜⬜⬜
	// [CASE 16]: Strictly Inside
	else if (
		rectangle.x() > hole.x() && 
		rectangle.x() < hole_x2 &&  
		rectangle.y() > hole.y() && 
		rectangle.y() < hole_y2 &&  
		rectangle_x2 > hole.x() && 
		rectangle_x2 < hole_x2 &&  
		rectangle_y2 > hole.y() && 
		rectangle_y2 < hole_y2     
	)
	{
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), hole_x2 - hole.x(), rectangle.y() - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), rectangle_y2, hole_x2 - hole.x(), hole_y2 - rectangle_y2));
		new_holes.push_back(Shape(next_hole_id++, hole.x(), hole.y(), rectangle.x() - hole.x(), hole_y2 - hole.y()));
		new_holes.push_back(Shape(next_hole_id++, rectangle_x2, hole.y(), hole_x2 - rectangle_x2, hole_y2 - hole.y()));
	}

	// [SPECIAL CASE] Hole is covered by rectangle
	// do nothing
	if (hole.is_covered(holes))
	{
		return;
	}

	for (auto new_hole : new_holes)
	{
		if (!new_hole.is_covered(holes))
		{
			holes.push_back(new_hole);
		}
	}
}

// Merge holes next to each other into bigger holes improving QoR
void merge_holes(std::vector<Shape> &holes)
{
	bool merged;
	do
	{
		merged = false;
		std::sort(holes.begin(), holes.end(), [](const Shape &a, const Shape &b)
				  {
			if (a.y() != b.y()) return a.y() < b.y();
			return a.x() < b.x(); });

		for (size_t i = 0; i < holes.size(); ++i)
		{
			for (size_t j = i + 1; j < holes.size(); ++j)
			{
				if (holes[i].y() == holes[j].y() && holes[i].h() == holes[j].h() && holes[i].x2() == holes[j].x())
				{
					holes[i] = Shape(holes[i].id(), holes[i].x(), holes[i].y(), holes[i].w() + holes[j].w(), holes[i].h());
					holes.erase(holes.begin() + j);
					merged = true;
					goto next_merge_pass;
				}
				if (holes[i].x() == holes[j].x() && holes[i].w() == holes[j].w() && holes[i].y2() == holes[j].y())
				{
					holes[i] = Shape(holes[i].id(), holes[i].x(), holes[i].y(), holes[i].w(), holes[i].h() + holes[j].h());
					holes.erase(holes.begin() + j);
					merged = true;
					goto next_merge_pass;
				}
			}
		}
	next_merge_pass:;
	} while (merged);
}

// Main method that splits holes into new holes and then merges holes
void update_holes(Shape &rectangle, std::vector<Shape> &holes, uint32_t &next_hole_id)
{
	std::vector<Shape> new_holes{};
	for (const Shape &hole : holes)
	{
		// If the Current Rectangle overlaps with a hole, we break the hole into new holes
		if (rectangle.intersects(hole))
		{
			cut_hole(rectangle, hole, new_holes, next_hole_id);
		}
		else if (!(hole.is_covered(new_holes)))
		{
			new_holes.push_back(hole);
		}
	}
	holes = std::move(new_holes);
	merge_holes(holes);
}

// gets y2 of rectangle if we were to place it in hole
uint32_t get_new_height(const Shape &hole, const Shape &rectangle)
{
	return hole.y() + rectangle.h();
}

// Differentiation function for get_best_hole, it says if this new hole is better than the current best hole
bool is_better_hole(const Shape &rectangle, const Shape &hole, const std::optional<Shape> &best_hole, float &best_dist)
{
	if (!rectangle.fits_in(hole)) return false;
	
	float target_x_clamped = std::max((float)hole.x(), std::min(rectangle.target_x(), (float)(hole.x2() - rectangle.w())));
	float target_y_clamped = std::max((float)hole.y(), std::min(rectangle.target_y(), (float)(hole.y2() - rectangle.h())));
	uint32_t nx = (uint32_t)std::round(target_x_clamped);
	uint32_t ny = (uint32_t)std::round(target_y_clamped);
	float dx = (float)nx - rectangle.target_x();
	float dy = (float)ny - rectangle.target_y();

	float dist = (dx * dx) + (dy * dy);

	if (!best_hole) return true;

	return dist < best_dist;
}

// Find the best hole to place our rectangle in
std::optional<Shape> get_best_hole(Shape &rectangle, const std::vector<Shape> &holes)
{
	std::optional<Shape> best_hole = std::nullopt;
	float best_dist = std::numeric_limits<float>::max();

	for (const Shape &hole : holes)
	{
		if (is_better_hole(rectangle, hole, best_hole, best_dist))
		{
            // Calculate distance again to update best_dist
            float dx = (float)hole.x() - rectangle.target_x();
            float dy = (float)hole.y() - rectangle.target_y();
			best_dist = (dx * dx) + (dy * dy);
            
			best_hole = hole;
		}
	}
	return best_hole;
}

void print_progress(uint32_t a, uint32_t b)
{
	if (b == 0)
		return;
	float percentage = (static_cast<float>(a) / b) * 100.0f;
	std::cout << "\r" << "  > Progress: " << a << "/" << b << " | " << std::fixed << std::setprecision(2) << percentage << "%";
}

// Main method to solve a macro placement instance
Result solve(uint32_t W, uint32_t H, std::vector<Shape> rectangles, Heuristic strategy, bool show_progress)
{
	// Initializations
	Result result{};
	result.w = W;
	result.h = H;

	std::vector<Shape> holes{};

	// Start Hole is the width of the entire canvas + an irrelevant height
	holes.push_back(Shape(1, 0, 0, W, H));

	// Time
	auto start = std::chrono::high_resolution_clock::now();

	// Sort based on heuristic
	switch (strategy)
	{
	case Heuristic::DescendingArea:
		std::sort(rectangles.begin(), rectangles.end(), descending_area);
		break;
	case Heuristic::DescendingArea2:
		std::sort(rectangles.begin(), rectangles.end(), descending_area_2);
		break;
	case Heuristic::DescendingWidth:
		std::sort(rectangles.begin(), rectangles.end(), descending_width);
		break;
	case Heuristic::DescendingHeight:
		std::sort(rectangles.begin(), rectangles.end(), descending_height);
		break;
	default:
		break;
	}

	uint32_t n = 0, N = rectangles.size();
	uint32_t next_hole_id = 0;

	// Verbose
	if (show_progress)
	{
		print_progress(n, N);
	}

	for (Shape &rectangle : rectangles)
	{
		std::optional<Shape> hole = get_best_hole(rectangle, holes);

		if (!hole)
		{
			throw std::runtime_error("No hole for rectangle " + std::to_string(rectangle.id()));
		}

		// Snap rectangle to the closest valid hole coordinate
		float target_x_clamped = std::max((float)hole->x(), std::min(rectangle.target_x(), (float)(hole->x2() - rectangle.w())));
		float target_y_clamped = std::max((float)hole->y(), std::min(rectangle.target_y(), (float)(hole->y2() - rectangle.h())));
		uint32_t nx = (uint32_t)std::round(target_x_clamped);
		uint32_t ny = (uint32_t)std::round(target_y_clamped);
		rectangle.set_position(nx, ny);

		// Update the holes map
		update_holes(rectangle, holes, next_hole_id);

		n++;
		if (show_progress) print_progress(n, N);
	}
	if (show_progress)
		std::cout << '\n';

	auto end = std::chrono::high_resolution_clock::now();

	// Save result
	result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	result.rectangles = rectangles;

	std::sort(result.rectangles.begin(), result.rectangles.end(), [](const Shape &a, const Shape &b)
			  { return a.id() < b.id(); });

#if CHECK_VALID
	bool passed_check = true;
	for (size_t i = 0; i < rectangles.size(); ++i)
	{
		for (size_t j = i + 1; j < rectangles.size(); ++j)
		{
			if (rectangles[i].intersects(rectangles[j]))
			{
				passed_check = false;
				break;
			}
		}
		if (!passed_check)
			break;
	}
	if (show_progress)
		std::cout << "Solution is " << (passed_check ? "valid" : "not valid") << '\n';
#endif

	return result;
}