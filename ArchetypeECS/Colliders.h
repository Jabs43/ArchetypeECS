#pragma once
#include "raymath.h"
#include <limits> // Required for infinity
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

class Collider {
public:
	enum class Shape : std::uint8_t {
		Box,
		Circle
	};

	// Default a 1 x 1 box
	Collider() = default;

	static Collider Box(float width, float height) {
		Collider collider;
		collider.SetAsBox(width, height);
		return collider;
	}

	static Collider Circle(float Radius) {
		Collider collider;
		collider.SetAsCircle(Radius);
		return collider;
	}

	void SetAsBox(float width, float height) {
		ValidateDimension(width);
		ValidateDimension(height);
		shape = Shape::Box;
		halfExtents = { width * 0.5f, height * 0.5f };
		radius = 0.0f;
	}

	void SetAsCircle(float Radius) {
		ValidateDimension(Radius);
		shape = Shape::Circle;
		radius = Radius;
		halfExtents = { 0.0f, 0.0f };
	}

	Shape GetShape() const {
		return shape;
	}

	Vector2 GetBoxHalfExtents() const {
		return halfExtents;
	}

	float GetCircleRadius() const {
		return radius;
	}

	// Half-extents of the world-aligned boinding box
	// Angle is in radians
	Vector2 BoundingHalfExtents(float worldRotation = 0.0f) const {
		if (shape == Shape::Circle) {
			return { radius, radius };
		}

		const float c = std::fabs(std::cos(worldRotation));
		const float s = std::fabs(std::sin(worldRotation));
		
		return {
			halfExtents.x * c + halfExtents.y * s,
			halfExtents.x * s + halfExtents.y * c
		};
	}
private:
	static void ValidateDimension(float value) {
		if (!std::isfinite(value) || value <= 0.0f) {
			throw std::invalid_argument("Collider dimensions must be finite and positive");
		}
	}

	Shape shape = Shape::Box;
	Vector2 halfExtents{ 0.5f,0.5f };
	float radius = 0.0f;
};

struct Contact {
	bool    hit = false;
	Vector2 normal{ 0, 0 };   // unit vector, points from A toward B
	float   depth = 0.0f;     // penetration depth along normal (>0 when hit)
};

struct Rotation {
	float radians = 0.0f;
};

struct RigidBody {
	float inversMass = 1.0f;// 0 == immovable in this simple model
	float restitution = 0.0f;
};

struct Point {
	float x, y;
};

struct Rect {
	float x, y, width, height;
	bool contains(const Vector2& p) const;
	bool intersects(const Rect& other) const;
};