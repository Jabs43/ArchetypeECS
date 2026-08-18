#pragma once
#include "ECS.h"
#include "SpatialGrid.h"
#include "raylib.h"
#include <vector>

namespace System {
	struct BoidParams {
		float viewRadius = 40.0f;
		float separationR = 20.0f;
		float maxSpeed = 120.0f;
		float maxForce = 200.0f;
		float wSep = 2.0, wAli = 0.6f, wCoh = 0.6f;
	};

	void BoidsSystem(ECS::ArchetypeRegistry& ecs, SpatialGrid& grid, const BoidParams& P, float dt, float worldW, float worldH) {
		static std::vector<GridItem> items;
		items.clear();

		ecs.ForEach<ECS::Position, ECS::Velocity, ECS::Boids>([&](ECS::EntityID e, ECS::Position& p, ECS::Velocity&, ECS::Boids&) {
			GridItem it{}; it.id = e; it.position = { p.x,p.y }; it.hw = it.hh = P.viewRadius;
			items.push_back(it);
		});

		std::vector<GridItem*> ptrs; ptrs.reserve(items.size());
		for (auto& it : items) ptrs.push_back(&it);
		grid.build(ptrs);

		static std::vector<GridItem*> nb;
		ecs.ForEach<ECS::Position, ECS::Velocity, ECS::Boids>([&](ECS::EntityID e, ECS::Position& p, ECS::Velocity& v, ECS::Boids& b) {
			nb.clear();
			GridItem self{ e, {p.x, p.y}, P.viewRadius, P.viewRadius };
			grid.getNearby(&self, nb);
			float sepX = 0, sepY = 0;
			float aliX = 0, aliY = 0;
			float cohX = 0, cohY = 0;
			int count = 0;

			for (GridItem* g : nb) {
				if (g->id == e) continue;
				ECS::Position& op = ecs.GetComponent<ECS::Position>(g->id);
				ECS::Velocity& ov = ecs.GetComponent<ECS::Velocity>(g->id);
				float dx = op.x - p.x, dy = op.y - p.y;
				float d2 = dx * dx + dy * dy;
				if (d2 > P.viewRadius * P.viewRadius || d2 == 0.0f) continue;

				aliX += ov.x; aliY += ov.y;
				cohX += op.x, cohY += op.y;
				if (d2 < P.separationR * P.separationR) {
					float inv = 1.0f / sqrtf(d2);
					sepX -= dx * inv; sepY -= dy * inv;
				}
				++count;
			}

			float fx = 0, fy = 0;
			if (count > 0) {
				aliX /= count; aliY /= count;
				cohX = cohX / count - p.x; cohY = cohY / count - p.y;
				fx = P.wSep * sepX + P.wAli * aliX + P.wCoh * cohX;
				fy = P.wSep * sepY + P.wAli * aliY + P.wCoh * cohY;
			}

			float f2 = fx * fx + fy * fy;
			if (f2 > P.maxForce * P.maxForce) { float s = P.maxForce / sqrtf(f2); fx *= s; fy *= s; }
			b.ax = fx; b.ay = fy;
		});

		ecs.ForEach<ECS::Position, ECS::Velocity, ECS::Boids>([&](ECS::EntityID, ECS::Position& p, ECS::Velocity& v, ECS::Boids& b) {
			v.x += b.ax * dt; v.y += b.ay * dt;
			float s2 = v.x * v.x + v.y * v.y;
			if (s2 > P.maxSpeed * P.maxSpeed) { float s = P.maxSpeed / sqrtf(s2); v.x *= s; v.y *= s; }
			p.x += v.x * dt; p.y += v.y * dt;
			// Infinite scroll
			if (p.x < 0) p.x += worldW; else if (p.x >= worldW) p.x -= worldW;
			if (p.y < 0) p.y += worldH; else if (p.y >= worldH) p.y -= worldH;
			b.ax = b.ay = 0;
		});
	}
}