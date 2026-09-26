#include "Colliders.h"

bool Rect::contains(const Vector2& p) const {
	return (p.x >= x && p.x <= x + width &&
		p.y >= y && p.y <= y + height);
}

bool Rect::intersects(const Rect& other) const {
	return !(other.x > x + width ||
		other.x + other.width < x ||
		other.y > y + height ||
		other.y + other.height < y);
}
