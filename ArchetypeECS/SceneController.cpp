#include "SceneController.h"

void SceneController::SwitchTo(const std::string& next) {
	if (!current.empty()) {
		scenes.at(current)->OnExit();
	}

	current = next;
	scenes.at(current)->OnEnter();
}
float SceneController::EaseInOutQuad(float t) {
	if (t < 0.5f) return 2.0f * t * t;
	return 1.0f - std::pow(-2.0f * t + 2.0f, 2) / 2.0f;
}

void SceneController::AddScene(std::string name, std::unique_ptr<IScene> scene) {
	scenes.emplace(std::move(name), std::move(scene));
}

void SceneController::AddTransition(std::string from, std::string to, Transition transition) {
	transitions[{std::move(from), std::move(to)}] = transition;
}

// Call once after registering scenes
void SceneController::Start(const std::string& name) { SwitchTo(name); }

void SceneController::RequestScene(const std::string& next) {
	if (transitioning || next == current ||
		scenes.find(next) == scenes.end()) {
		return;
	}

	// Queue the request; never exit a scene inside its update callback
	destination = next;
}

void SceneController::Update(float dt) {
	if (transitioning) {
		elapsed = std::min(elapsed + dt, animation.duration);

		const float switchProgress =
			animation.effect == TransitionEffect::BUBBLE
			? 2.55f / 4.0f
			: 0.5;
		const float midpoint = animation.duration * switchProgress;

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

void SceneController::Draw(const int width, const int height) {
	scenes.at(current)->Draw();
	if (!transitioning) return;
	const float progress = elapsed / animation.duration;
	// Coveragte increases 0 -> 1, then decreases 1 -> 0
	const float coverage = progress < 0.5f
		? progress * 2.0f
		: (1.0 - progress) * 2.0f;

	switch (animation.effect) {
	case TransitionEffect::FADE:
		DrawRectangle(0, 0, width, height, Fade(animation.color, coverage));
		break;

	case TransitionEffect::WIPE: {
		// Cover from the left, then uncover towards the right.
		const float edge = progress < 0.5f
			? EaseInOutQuad(progress * 2.0f)
			: EaseInOutQuad((progress - 0.5f) * 2.0f);

		const int pixels = static_cast<int>(width * edge);
		if (progress < 0.5f) {
			DrawRectangle(0, 0, pixels, height, animation.color);
		}
		else {
			DrawRectangle(0, 0, width - pixels, height, animation.color);
		}

		break;
	}
	case TransitionEffect::BUBBLE: {
		constexpr int CellSize = 200;
		constexpr float TwoPi = PI * 2.0f;

		const float t = progress * 4.0f;
		const auto picoSin = [](float turns) {
			return -std::sin(turns * TwoPi);
			};

		const int columns = (width + CellSize - 1) / CellSize;
		const int rows = (height + CellSize - 1) / CellSize;

		constexpr float movement = CellSize * (10.0f / 20.0f);
		constexpr float maxRadius = CellSize * (15.0f / 16.0f);

		for (int column = 0; column < columns; ++column) {
			const float x = static_cast<float>(column * CellSize);
			const float osc1 = picoSin(t + column * 0.1f);
			for (int row = 0; row < rows; ++row) {
				const float osc2 = picoSin(t / 4.0f + row * 0.03f);
				const float y = row * CellSize + osc1 * movement;
				const float radius = osc2 * maxRadius;
				if (radius > 0.0f) {
					DrawCircleV({ x,y }, radius, animation.color);
				}
			}
		}
		break;
	}
	}
}

void SceneController::Shutdown() {
	if (!current.empty()) {
		scenes.at(current)->OnExit();
		current.clear();
	}
}
