#ifndef PACKER_H
#define PACKER_H

#include "types.h"

Result solve(uint32_t W, uint32_t H, std::vector<Shape> rectangles, Heuristic strategy, bool show_progress);

#endif