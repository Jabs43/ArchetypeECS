#pragma once
#include "raylib.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <cmath>

class SceneController;

class IScene {
protected:
	SceneController& controller;
public:
	IScene(SceneController& _controller) : controller(_controller) {}
	virtual ~IScene() = default;

	virtual void OnEnter() {}
	virtual void OnExit() {}

	virtual void Update(float dt) = 0;
	virtual void Draw() = 0;
};

enum class TransitionEffect { FADE, WIPE, BUBBLE };

struct Transition {
	TransitionEffect effect;
	float duration;
	Color color;
};

class SceneController {
	using ScenePair = std::pair<std::string, std::string>;

	std::map<std::string, std::unique_ptr<IScene>> scenes;
	std::map<ScenePair, Transition> transitions;

	std::string current;
	std::string destination;

	Transition animation{ TransitionEffect::FADE, 0.6f, BLACK };
	float elapsed = 0.0f;
	bool transitioning = false;
	bool switched = false;

	void SwitchTo(const std::string& next);
	float EaseInOutQuad(float t);

public:

	void AddScene(std::string name, std::unique_ptr<IScene> scene);
	void AddTransition(std::string from, std::string to, Transition transition);

	// Call once after registering scenes
	void Start(const std::string& name);

	void RequestScene(const std::string& next);

	void Update(float dt);

	void Draw(const int width, const int height);

	void Shutdown();
};