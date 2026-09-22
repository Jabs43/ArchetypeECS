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

	void BoidsSystem(ECS::ArchetypeRegistry& ecs, SpatialGrid& grid, const BoidParams& P, float dt, float worldW, float worldH);
}