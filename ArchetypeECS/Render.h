#pragma once
#include "raylib.h"

class Render {
private:
	RenderTexture2D target{};
public:
	Render(int width, int height);
	~Render();

	Render(const Render&) = delete;
	Render& operator=(const Render&) = delete;
	Render(Render&&) = delete;
	Render& operator=(Render&&) = delete;

	bool SetResolution(int width, int height);
	bool SetAspectRation(float width, float height);
	int GetGameScreenWidth() const { return target.texture.width; }
	int GetGameScreenHeight() const { return target.texture.height; }
	Vector2 GetGameScreenV() const;
	float GetAspectRatio() const;
	float GetScale() const;
	Rectangle GetViewport() const;

	// Refresh before state input and GUI drawing. GetMousePosition/GetMouseDelta
	// then use game coordinates globally; do not convert them a second time.
	void UpdateMouseMapping() const;
	// Compatibility helper: returns the already-mapped mouse position.
	Vector2 CalcVirtualMouse() const;
	bool IsMouseInsideGame() const;

	void Begin() const;
	void End() const;
	void DrawRenderTexture() const;

	// Pure helpers also work without an open window (used by the scaling tests)
	static Rectangle CalculateViewport(int windowWidth, int windowHeight, int gameWidth, int gameHeight);
	static Vector2 WindowToGame(Vector2 point, Rectangle viewport, int gameWidth, int gameHeight);
};
