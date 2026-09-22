#pragma once
#include "raymath.h"
#include <limits> // Required for infinity
#include <cmath>
#include <algorithm>
#include <cstdint>

struct Point {
	float x, y;
};

struct Rect {
	float x, y, width, height;
	bool contains(const Point& p) const;
	bool intersects(const Rect& other) const;
};