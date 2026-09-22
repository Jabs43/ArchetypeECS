#include "Colliders.h"

inline bool Rect::contains(const Point& p) const {
	return (p.x >= x && p.x <= x + width &&
		p.y >= y && p.y <= y + height);
}

inline bool Rect::intersects(const Rect& other) const {
	return !(other.x > x + width ||
		other.x + other.width < x ||
		other.y > y + height ||
		other.y + other.height < y);
}
