#include "raylib.h"
#include "Render.h"
#include "ECS.h"
#include "SpatialGrid.h"
#include "Boids.h"
#include <vector>
#include <cstdint>

using namespace ECS;

int main(void) {

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Raylib Template");
	if (!IsWindowReady()) return 1;

	Render render(640, 360);
	const int GameScreenWidth = render.GetGameScreenWidth();
	const int GameScreenHeight = render.GetGameScreenHeight();

	ArchetypeRegistry ecs;
	ecs.RegisterComponent<Position>();
	ecs.RegisterComponent<Velocity>();
	ecs.RegisterComponent<Boids>();

	std::vector<EntityID> es;
	System::BoidParams params{};
	for (int i = 0; i < 1'000; ++i) {
		EntityID e = ecs.CreateEntity();
		float ang = (float)GetRandomValue(0, 359) * DEG2RAD;   // raylib helpers, no extra include
		float spd = params.maxSpeed * 0.5f;
		ecs.AddComponent<Position>(e, { (float)GetRandomValue(0, GameScreenWidth),
										(float)GetRandomValue(0, GameScreenHeight) });
		ecs.AddComponent<Velocity>(e, { cosf(ang) * spd, sinf(ang) * spd });
		ecs.AddComponent<Boids>(e, { 0.f, 0.f });
		es.push_back(e);
	}

	printf("Archetypes after build: %zu\n", ecs.ArchetypeCount());

	SpatialGrid grid(GameScreenWidth, GameScreenHeight, params.viewRadius);

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		System::BoidsSystem(
			ecs,
			grid,
			params,
			dt,
			GameScreenWidth,
			GameScreenHeight
		);

		render.UpdateMouseMapping();
		render.Begin();

		ClearBackground(DARKGRAY);
		DrawText("Hello World", int(GameScreenWidth * .5f) - (10 * 11), int(GameScreenHeight * .5f), 40, LIGHTGRAY);

		ecs.ForEach<Position>([&](EntityID, Position& p) {
			DrawCircle((int)p.x, (int)p.y, 5, BLUE);
			});

		DrawFPS(0, 0);
		
		render.End();
		render.DrawRenderTexture();
	}

	// De-init textures -----------------------------------------------
	CloseWindow();
	return 0;
}