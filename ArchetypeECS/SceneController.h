#pragma once
#include "raylib.h"
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>

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

enum class TransitionEffect { FADE, WIPE };

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

	void SwitchTo(const std::string& next) {
		if (!current.empty()) {
			scenes.at(current)->OnExit();
		}

		current = next;
		scenes.at(current)->OnEnter();
	}
public:
	void AddScene(std::string name, std::unique_ptr<IScene> scene) {
		scenes.emplace(std::move(name), std::move(scene));
	}

	void AddTransition(std::string from, std::string to, Transition transition) {
		transitions[{std::move(from), std::move(to)}] = transition;
	}

	// Call once after registering scenes
	void Start(const std::string& name) {
		SwitchTo(name);
	}

	void RequestScene(const std::string& next) {
		if (transitioning || next == current ||
			scenes.find(next) == scenes.end()) {
			return;
		}
	
		// Queue the request; never exit a scene inside its update callback
		destination = next;
	}

	void Update(float dt) {
		if (transitioning) {
			elapsed = std::min(elapsed + dt, animation.duration);

			const float midpoint = animation.duration * 0.5f;

			if (!switched && elapsed >= midpoint) {
				SwitchTo(destination);
				switched = true;
			}

			if (elapsed >= animation.duration) {
				transitioning = false;
				destination.clear();
			}

			// Freeze scene update throughout the transition
			return;
		}

		scenes.at(current)->Update(dt);

		if (destination.empty()) return;
		
		const auto found = transitions.find({ current, destination });

		// Unregistered pairs switch immediately.
		if (found == transitions.end() || found->second.duration <= 0.0f) {
			SwitchTo(destination);
			destination.clear();
			return;
		}

		animation = found->second;
		elapsed = 0.0f;
		switched = false;
		transitioning = true;
	}

	void Draw() {
		scenes.at(current)->Draw();
		if (!transitioning) return;
		const float progress = elapsed / animation.duration;
		// Coveragte increases 0 -> 1, then decreases 1 -> 0
		const float coverage = progress < 0.5f
			? progress * 2.0f
			: (1.0 - progress) * 2.0f;

		const int width = GetScreenWidth();
		const int height = GetScreenHeight();

		switch (animation.effect) {
			case TransitionEffect::FADE:
				DrawRectangle(0, 0, width, height, Fade(animation.color, coverage));
				break;

			case TransitionEffect::WIPE: {
				// Cover from the left, then uncover towards the right.
				const float edge = progress < 0.5f
					? progress * 2.0f
					: (progress - 0.5f) * 2.0f;

				const int pixels = static_cast<int>(width * edge);
				if (progress < 0.5f) {
					DrawRectangle(0, 0, pixels, height, animation.color);
				}
				else {
					DrawRectangle(0,0,width-pixels,height,animation.color);
				}

				break;
			}
		}
	}

	void Shutdown() {
		if (!current.empty()) {
			scenes.at(current)->OnExit();
			current.clear();
		}
	}
};