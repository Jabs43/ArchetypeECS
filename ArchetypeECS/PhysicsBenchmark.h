#pragma once
#include "ECS.h"
#include "PhysicsSystem.h"
#include "raylib.h"
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

#ifdef RUN_BENCHMARK_PHYSICS
#pragma message(">>> PHYSICS main compiling")
#endif

#ifdef RUN_BENCHMARK_PHYSICS

using clk = std::chrono::steady_clock;
static double ms_since(clk::time_point t0) {
	return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

// -- Distribution of spawn positions --
enum class Dist { Uniform, Clustered };

// -- Result of one config -- 
struct BenchResult {
	int count = 0;
	double physMean = 0, physP50 = 0, physP95 = 0, physP99 = 0, physMax = 0;
	double ctrlMean = 0;
	double cacheEff = -1;
	int outOfBounds = 0;
	int colliding = 0;
};

static double percentile(std::vector<double>& v, double p) {
	if (v.empty()) return 0;
	std::sort(v.begin(), v.end());
	size_t idx = (size_t)std::clamp(p * (v.size() - 1), 0.0, (double)(v.size() - 1));
	return v[idx];
}

static ECS::EntityID addBody(ECS::ArchetypeRegistry& ecs,
	Vector2 p, Vector2 v, Collider collider,
	float inverseMass, float restitution) {
	const auto id = ecs.CreateEntity();
	ecs.AddComponent<ECS::Position>(id, { p.x, p.y });
	ecs.AddComponent<ECS::Velocity>(id, { v.x, v.y });
	ecs.AddComponent<Rotation>(id, {});
	ecs.AddComponent<Collider>(id, collider);
	ecs.AddComponent<RigidBody>(id, { inverseMass, restitution });
	ecs.AddComponent<CollisionState>(id, {});
	return id;
}

static BenchResult runBench(int count, int frames, Dist dist, bool keepDensityConstant,
	const char* profileTag = nullptr, float dt = 1.0f / 60.0f) {
	BenchResult r; r.count = count;

	// Arena: fixed 1920x1080, or scaled to hold density constant.
	float w = 1920, h = 1080;
	if (keepDensityConstant) {
		float scale = std::sqrt((float)count / 1000.0f);
		w = 1920.0f * scale; h = 1080.0f * scale;
	}

	ECS::ArchetypeRegistry ecs_;
	ecs_.RegisterComponent<ECS::Position>();
	ecs_.RegisterComponent<ECS::Velocity>();
	ecs_.RegisterComponent<Rotation>();
	ecs_.RegisterComponent<Collider>();
	ecs_.RegisterComponent<RigidBody>();
	ecs_.RegisterComponent<CollisionState>();

	Rect arena_{ 0,0, w,h };
	System::PhysicsSystem physics_{ arena_ };

	std::mt19937 rng(1234);// fixed seed => reproducible runs
	std::uniform_real_distribution<float> ux(1, w - 1), uy(1, h - 1);
	std::normal_distribution<float> cx(w * 0.5f, w * 0.02f), cy(h * 0.5f, h * 0.02f);
	std::uniform_real_distribution<float> uv(-150.f, 150.f);

	//std::vector<ECS::EntityID> ids; ids.reserve(count);
	for (int i = 0; i < count; ++i) {
		float x = (dist == Dist::Uniform) ? ux(rng) : std::clamp(cx(rng), 1.f, w - 1);
		float y = (dist == Dist::Uniform) ? uy(rng) : std::clamp(cy(rng), 1.f, h - 1);
		const auto id = addBody(ecs_, {x, y}, {uv(rng), uv(rng)}, Collider::Box(10, 10), 1.0f, 0.0f);
		//ids.push_back(id);
	}

	for (int f = 0; f < 5; ++f) { physics_.Update(ecs_, dt); }

#ifdef PHYS_PROFILE
	if (profileTag) physics_.ResetProfile();
#endif

	std::vector<double> physTimes; physTimes.reserve(frames);
	for (int f = 0; f < frames; ++f) {
		auto t0 = clk::now();
		physics_.Update(ecs_, dt);
		physTimes.push_back(ms_since(t0));
	}

#ifdef PHYS_PROFILE
	if (profileTag) physics_.ReportProfile(profileTag);
#endif

	r.physMean = std::accumulate(physTimes.begin(), physTimes.end(), 0.0) / physTimes.size();
	r.physP50 = percentile(physTimes, 0.50);
	r.physP95 = percentile(physTimes, 0.95);
	r.physP99 = percentile(physTimes, 0.99);
	r.physMax = *std::max_element(physTimes.begin(), physTimes.end());

	ecs_.ForEach<ECS::Position, ECS::Velocity, Rotation, Collider, RigidBody, CollisionState>(
		[&](ECS::EntityID, ECS::Position& p, ECS::Velocity&,
			Rotation&, Collider&, RigidBody&, CollisionState& cs) {
			if (p.x < arena_.x || p.x > arena_.x + arena_.width ||
				p.y < arena_.y || p.y > arena_.y + arena_.height)
				r.outOfBounds++;
			if (cs.hit) r.colliding++;
		});

	return r;
}

static void printTable(const char* title, const std::vector<BenchResult>& rows) {
	printf("\n=== %s ===\n", title);
	printf("%8s | %9s %9s %9s %9s %9s | %8s | %7s %7s\n",
		"count", "phys_mean", "p50", "p95", "p99", "max(ms)",
		"ctrl_ms", "cacheEff", "oob");
	printf("---------+---------------------------------------------------+----------+-----------------\n");
	for (const auto& r : rows) {
		printf("%8d | %9.3f %9.3f %9.3f %9.3f %9.3f | %8.4f | ",
			r.count, r.physMean, r.physP50, r.physP95, r.physP99, r.physMax, r.ctrlMean);
		if (r.cacheEff >= 0) printf("%6.1f%% ", r.cacheEff * 100.0);
		else                 printf("   n/a  ");
		printf("| %5d\n", r.outOfBounds);
	}
	printf("(16.67ms = 60fps budget, 8.33ms = 120fps. oob must stay 0.)\n");
}

int main() {
	SetConfigFlags(FLAG_WINDOW_HIDDEN);          // valid GL context, never presented
	InitWindow(64, 64, "bench");
	SetTraceLogLevel(LOG_WARNING);               // quiet raylib chatter

	const int frames = 600;                      // ~10s of 60fps-equivalent work
	const std::vector<int> counts = { 100, 500, 1000, 2500, 5000, 10000 };
	// NOTE: keep max <= your MAX_ENTITIES. Trim this list if you assert.

	std::vector<BenchResult> uniformConst, uniformFixed, clustered;
	for (int c : counts) {
		uniformConst.push_back(runBench(c, frames, Dist::Uniform,  /*constDensity*/true));
		uniformFixed.push_back(runBench(c, frames, Dist::Uniform,  /*constDensity*/false));
		clustered.push_back(runBench(c, frames, Dist::Clustered,  /*constDensity*/false,
			(c == 10000) ? "CLUSTERED 10k" : nullptr));
	}

	printTable("Uniform, CONSTANT density (isolates N -- expect ~linear)", uniformConst);
	printTable("Uniform, FIXED arena (density rises with N -- realistic crowding)", uniformFixed);
	printTable("CLUSTERED in fixed arena (quadtree worst case -- expect superlinear)", clustered);

	std::getchar();   // keep the console open until you press Enter
	CloseWindow();
	return 0;
}

#endif