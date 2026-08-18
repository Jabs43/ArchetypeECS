#pragma once
#include "raylib.h"
#include <iostream>
#include <algorithm>// std::min

class Render {
	Render() { std::cout << "Render Singleton Initialized.\n"; }
	~Render() = default;

	// Rendering settings 
	float AsRatio = float(16.0f / 9.0f);

	int GameScreenWidth = 720;
	int GameScreenHeight = int(720 / (16.0f / 9.0f));

	int ScreenWidth = 450;
	int ScreenHeight = int(450 / (16.0f / 9.0f));

	RenderTexture2D target;

public:
	// Delete copy constructor and assigment operators to prevent duplicates
	Render(const Render&) = delete;
	Render& operator=(const Render&) = delete;
	Render(Render&&) = delete;
	Render& operator=(Render&&) = delete;

	// public static fn to acces the single instance
	static Render& GetInstance();

	void SetAspectRatio(float width, float height);
	void SetResolution(int resolution);
	float GetAspectRatio() const;
	int GetGameScreenHeight() const;
	int GetGameScreenWidth() const;
	Vector2 GetGameScreenV() const;
	RenderTexture2D GetRenderTexture() const;
	float GetScale() const;
	Vector2 CalcVirtualMouse() const;
	void InitRaylibWindow(int wWidth, int wHeight, int resolution, float ratio);
	void DrawRenderTexture();
	void CloseRaylibWindow();
};