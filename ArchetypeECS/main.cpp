#include "raylib.h"
#include "Render.h"
#include "ECS.h"
#include <vector>
#include <cstdint>

using namespace ECS;

int main(void) {

	const float AsRatio = (float)(16.0f / 9.0f);
	const int ScreenWidth = 450;
	const int ScreenHeight = (int)(ScreenWidth / AsRatio);

	// Init render singleton
	Render::GetInstance().InitRaylibWindow(ScreenWidth, ScreenHeight, 1080, AsRatio);
	SetTraceLogLevel(LOG_WARNING);// Quiet raylib chatter

	const int GameScreenWidth = Render::GetInstance().GetGameScreenWidth();
	const int GameScreenHeight = Render::GetInstance().GetGameScreenHeight();

	ArchetypeRegistry ecs;
	ecs.RegisterComponent<Position>();
	ecs.RegisterComponent<Velocity>();
	ecs.RegisterComponent<Interactable>();

	std::vector<EntityID> es;
	for (int i = 0; i < 6; ++i) {
		EntityID e = ecs.CreateEntity();
		
		ecs.AddComponent<Position>(e, { float(i), float(i)});
		if ((i & 2) == 0) ecs.AddComponent<Velocity>(e, { 1.0f, -1.0f });
		if ((i & 3) == 0) ecs.AddComponent<Interactable>(e, { true });
		es.push_back(e);
	}

	printf("Archetypes after build: %zu\n", ecs.ArchetypeCount());

	ecs.ForEach<Position, Velocity>([](EntityID e, Position& p, Velocity& v) {
		p.x += v.x; p.y += v.y;
		printf(" step e%u -> (%.1f, %.1f)\n", e, p.x, p.y);
	});

	ecs.RemoveComponent<Velocity>(es[0]);
	ecs.AddComponent<Velocity>(es[1], { 9.0f, 9.0f });
	ecs.DestroyEntity(es[4]);

	printf("Archetypes after churn: &zu\n", ecs.ArchetypeCount());

	for (EntityID e : es) {
		if (!ecs.Has(e)) { printf(" e%u destroyed\n", e); continue; }
		Position& p = ecs.GetComponent<Position>(e);
		printf(" e%u Position=(%.1f, %.1f)%s\n", e, p.x, p.y,
			e == es[0] ? " [no longer has velocity]" : "");
	}

	printf("e0 Has? %d ; ForEach<p,v> now visits:\n", (int)ecs.Has(es[0]));
	ecs.ForEach<Position, Velocity>([&](EntityID e, Position&, Velocity& v) {
		printf(" e%u vel=(%.1f, %.1f)\n", e, v.x, v.y);
		});

	while (!WindowShouldClose()) {
		// Scale calc -----------------------------------------------------
		float Scale = Render::GetInstance().GetScale();
		float xoff = (GetScreenWidth() - ((float)GameScreenWidth * Scale)) * .5f;
		float yoff = (GetScreenHeight() - ((float)GameScreenHeight * Scale)) * .5f;
		SetMouseOffset(-xoff, -yoff);
		SetMouseScale(1.0f / Scale, 1.0f / Scale);

		RenderTexture2D target = Render::GetInstance().GetRenderTexture();
		// Draw things to be scaled --------------------------------------
		BeginTextureMode(target);
			ClearBackground(BLACK);
			DrawText("Hello World", int(GameScreenWidth * .5f), int(GameScreenHeight * .5f), 40, LIGHTGRAY);
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