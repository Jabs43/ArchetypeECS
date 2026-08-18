#include "raylib.h"
#include "Render.h"

int main(void) {

	const float AsRatio = (float)(16.0f / 9.0f);
	const int ScreenWidth = 450;
	const int ScreenHeight = (int)(ScreenWidth / AsRatio);

	// Init render singleton
	Render::GetInstance().InitRaylibWindow(ScreenWidth, ScreenHeight, 1080, AsRatio);
	SetTraceLogLevel(LOG_WARNING);// Quiet raylib chatter

	const int GameScreenWidth = Render::GetInstance().GetGameScreenWidth();
	const int GameScreenHeight = Render::GetInstance().GetGameScreenHeight();

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