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
	bool contains(const Point& p) const {
		return (p.x >= x && p.x <= x + width &&
			p.y >= y && p.y <= y + height);
	}

	bool intersects(const Rect& other) const {
		return !(other.x > x + width ||
			other.x + other.width < x ||
			other.y > y + height ||
			other.y + other.height < y);
	}
};