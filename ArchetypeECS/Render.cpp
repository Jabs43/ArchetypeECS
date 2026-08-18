#include "Render.h"


Render& Render::GetInstance() {
	static Render instance;
	return instance;
}

void Render::SetAspectRatio(float width, float height) {
	AsRatio = float(width / height);
	GameScreenHeight = int(GameScreenWidth / AsRatio);
}

void Render::SetResolution(int resolution) {
	GameScreenWidth = resolution;
	GameScreenHeight = int(GameScreenWidth / AsRatio);
}

float Render::GetAspectRatio() const {
	return AsRatio;
}

int Render::GetGameScreenHeight() const {
	return GameScreenHeight;
}

int Render::GetGameScreenWidth() const {
	return GameScreenWidth;
}

Vector2 Render::GetGameScreenV() const {
	return { (float)GameScreenWidth, (float)GameScreenHeight };
}

RenderTexture2D Render::GetRenderTexture() const {
	return target;
}

float Render::GetScale() const {
	return std::min((float)GetScreenWidth() / GameScreenWidth,
		(float)GetScreenHeight() / GameScreenHeight);
}

Vector2 Render::CalcVirtualMouse() const {

	float Scale = GetScale();
	float xoff = (GetScreenWidth() - (GameScreenWidth * Scale)) * .5f;
	float yoff = (GetScreenHeight() - (GameScreenHeight * Scale)) * .5f;
	Vector2 mouseRaw = GetMousePosition();

	return { (mouseRaw.x - xoff) / Scale, (mouseRaw.y - yoff) / Scale };
}

void Render::InitRaylibWindow(int wWidth, int wHeight, int resolution, float ratio) {
	SetResolution(resolution);
	ScreenWidth = wWidth;
	ScreenHeight = wHeight;

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(ScreenWidth, ScreenHeight, "raylib exmple - basic window");
	SetTargetFPS(144);
	SetWindowMinSize(320, 240);

	target = LoadRenderTexture(GameScreenWidth, GameScreenHeight);
	SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
}

void Render::DrawRenderTexture() {
	float Scale = GetScale();

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

void Render::CloseRaylibWindow() {
	UnloadRenderTexture(target);
	CloseWindow();
}
