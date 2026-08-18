#include "raylib.h"
#include "Render.h"
#include "ECS.h"
#include "SpatialGrid.h"
#include "Boids.h"
#include <vector>
#include <cstdint>

using namespace ECS;

int main(void) {

	const float AsRatio = (float)(16.0f / 9.0f);
	const int ScreenWidth = 450;
	const int ScreenHeight = (int)(ScreenWidth / AsRatio);

	// Init render singleton
	Render::GetInstance().InitRaylibWindow(ScreenWidth, ScreenHeight, 1440, AsRatio);
	SetTraceLogLevel(LOG_WARNING);// Quiet raylib chatter

	const int GameScreenWidth = Render::GetInstance().GetGameScreenWidth();
	const int GameScreenHeight = Render::GetInstance().GetGameScreenHeight();

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
		// Scale calc -----------------------------------------------------
		float Scale = Render::GetInstance().GetScale();
		float xoff = (GetScreenWidth() - ((float)GameScreenWidth * Scale)) * .5f;
		float yoff = (GetScreenHeight() - ((float)GameScreenHeight * Scale)) * .5f;
		SetMouseOffset(-xoff, -yoff);
		SetMouseScale(1.0f / Scale, 1.0f / Scale);
		float dt = GetFrameTime();

		//ecs.ForEach<Position, Velocity>([&](EntityID e, Position& p, Velocity& v) {
		//	p.x += v.x * dt; p.y += v.y * dt;
		//	printf(" step e%u -> (%.1f, %.1f)\n", e, p.x, p.y);
		//});

		System::BoidsSystem(
			ecs,
			grid,
			params,
			dt,
			GameScreenWidth,
			GameScreenHeight
		);

		RenderTexture2D target = Render::GetInstance().GetRenderTexture();
		// Draw things to be scaled --------------------------------------
		BeginTextureMode(target);
			ClearBackground(DARKGRAY);
			DrawText("Hello World", int(GameScreenWidth * .5f) - (10 * 11), int(GameScreenHeight * .5f), 40, LIGHTGRAY);

			ecs.ForEach<Position>([&](EntityID, Position& p) {
				DrawCircle((int)p.x, (int)p.y, 5, BLUE);
			});

			DrawFPS(0, 0);
		EndTextureMode();

		// Draw render texture to screen properly scaled ----------------
		BeginDrawing();
			ClearBackground(BLACK);
			DrawTexturePro(
				target.texture,
				Rectangle{ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
				Rectangle{ (GetScreenWidth() - ((float)GameScreenWidth * Scale)) * 0.5f,
				(GetScreenHeight() - ((float)GameScreenHeight * Scale)) * 0.5f,
				(float)GameScreenWidth * Scale, (float)GameScreenHeight * Scale },
				Vector2{ 0, 0 }, 0.0f, WHITE);
		EndDrawing();
	}

	// De-init textures -----------------------------------------------
	Render::GetInstance().CloseRaylibWindow();
	return 0;
}