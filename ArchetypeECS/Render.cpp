#include "Render.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Render::Render(int width, int height) {
	if (!SetResolution(width, height)) {
		throw std::runtime_error("Unable to create the game render texture.");
	}
}

Render::~Render() {
	SetMouseOffset(0, 0);
	SetMouseScale(1.0f, 1.0f);
	if (target.id != 0) UnloadRenderTexture(target);
}

bool Render::SetResolution(int width, int height) {
	// Call between frames, never while this render target is bound 
	if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return false;
	if (width == GetGameScreenWidth() && height == GetGameScreenHeight()) return true;

	RenderTexture2D replacement = LoadRenderTexture(width, height);
	if (!IsRenderTextureValid(replacement)) {
		if (replacement.id != 0) UnloadRenderTexture(replacement);
		return false;
	}
	SetTextureFilter(replacement.texture, TEXTURE_FILTER_POINT);
	if (target.id != 0) UnloadRenderTexture(target);
	target = replacement;
	UpdateMouseMapping();
	return true;
}

bool Render::SetAspectRation(float width, float height) {
	if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
		return false;
	const double newHeight = GetGameScreenWidth() * (static_cast<double>(height) / width);
	if (!std::isfinite(newHeight) || newHeight < 1 || newHeight > 8192) return false;
	return SetResolution(GetGameScreenWidth(), static_cast<int>(std::lround(newHeight)));
}

Vector2 Render::GetGameScreenV() const {
	return { static_cast<float>(GetGameScreenWidth()), static_cast<float>(GetGameScreenHeight()) };
}

float Render::GetAspectRatio() const {
	return static_cast<float>(GetGameScreenWidth()) / GetGameScreenHeight();
}

float Render::GetScale() const {
	return GetViewport().width / GetGameScreenWidth();
}

Rectangle Render::GetViewport() const {
	return CalculateViewport(GetScreenWidth(), GetScreenHeight(), GetGameScreenWidth(), GetGameScreenHeight());
}

void Render::UpdateMouseMapping() const {
	const Rectangle viewport = GetViewport();
	if (viewport.width <= 0 || viewport.height <= 0) {
		SetMouseOffset(0, 0);
		SetMouseScale(0.0f, 0.0f);
		return;
	}

	// raylib applies (rawMouse + offset) * scale, including inside raygui.
	// The viewport origin already includes centering/letterboxing.
	SetMouseOffset(-static_cast<int>(viewport.x), -static_cast<int>(viewport.y));
	SetMouseScale(GetGameScreenWidth() / viewport.width,
		GetGameScreenHeight() / viewport.height);
}

Vector2 Render::CalcVirtualMouse() const {
	const Rectangle viewport = GetViewport();
	if (viewport.width <= 0 || viewport.height <= 0) return { -1.0f, -1.0f };
	return GetMousePosition();
}

bool Render::IsMouseInsideGame() const {
	const Rectangle viewport = GetViewport();
	const Vector2 mouse = GetMousePosition();
	return viewport.width > 0 && viewport.height > 0 &&
		mouse.x >= 0 && mouse.x < GetGameScreenWidth() &&
		mouse.y >= 0 && mouse.y < GetGameScreenHeight();
}

void Render::Begin() const { BeginTextureMode(target); }
void Render::End() const { EndTextureMode(); }
void Render::DrawRenderTexture() const {
	BeginDrawing();

	ClearBackground(BLACK);
	const Rectangle viewport = GetViewport();
	if (viewport.width > 0 && viewport.height > 0) {
		// Render textures are vertically inverted in OpenGL
		DrawTexturePro(target.texture,
			Rectangle{ 0, 0, static_cast<float>(target.texture.width),
							-static_cast<float>(target.texture.height) },
			viewport, Vector2{ 0,0 }, 0, WHITE);
	}
	EndDrawing();
}

Rectangle Render::CalculateViewport(int windowWidth, int windowHeight, int gameWidth, int gameHeight) {
	if (windowWidth <= 0 || windowHeight <= 0 || gameWidth <= 0 || gameHeight <= 0)
		return {};
	const float scale = std::min(static_cast<float>(windowWidth) / gameWidth,
		static_cast<float>(windowHeight) / gameHeight);
	const float width = gameWidth * scale;
	const float height = gameHeight * scale;
	// Mouse offsets are integers in raylib. Align drawing to the same pixel origin
	// so odd window sizes cannot introduce a fractional drawing/input mismatch.
	return { std::floor((windowWidth - width) * 0.5f),
			 std::floor((windowHeight - height) * 0.5f), width, height };
}

Vector2 Render::WindowToGame(Vector2 point, Rectangle viewport, int gameWidth, int gameHeight)
{
	if (viewport.width <= 0 || viewport.height <= 0 || gameWidth <= 0 || gameHeight <= 0)
		return { -1.0f, -1.0f };
	return { (point.x - viewport.x) * gameWidth / viewport.width,
			 (point.y - viewport.y) * gameHeight / viewport.height };
}
