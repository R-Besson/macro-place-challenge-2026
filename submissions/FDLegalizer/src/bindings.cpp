#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "packer.h"

namespace py = pybind11;

PYBIND11_MODULE(macro_packer, m) {
	m.doc() = "C++ Macro Placement Legalizer";

	py::enum_<Heuristic>(m, "Heuristic")
		.value("DescendingArea", Heuristic::DescendingArea)
		.value("DescendingArea2", Heuristic::DescendingArea2)
		.value("DescendingWidth", Heuristic::DescendingWidth)
		.value("DescendingHeight", Heuristic::DescendingHeight)
		.export_values();

	py::class_<Shape>(m, "Shape")
		.def(py::init<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>())
		.def("id", &Shape::id)
		.def("x", &Shape::x)
		.def("y", &Shape::y)
		.def("w", &Shape::w)
		.def("h", &Shape::h)
		.def("target_x", &Shape::target_x)
		.def("target_y", &Shape::target_y)
		.def("set_target", &Shape::set_target)
		.def("set_position", &Shape::set_position);

	py::class_<Result>(m, "Result")
		.def_readonly("w", &Result::w)
		.def_readonly("h", &Result::h)
		.def_readonly("elapsed_ms", &Result::elapsed_ms)
		.def_readonly("rectangles", &Result::rectangles);

	m.def("solve", &solve, "Solve Legalization",
		py::arg("W"), py::arg("H"), py::arg("rectangles"), py::arg("strategy"), py::arg("show_progress"));
}