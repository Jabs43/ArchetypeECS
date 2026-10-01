#pragma once
#include "raylib.h"
#include "SceneController.h"


#ifdef RUN_SCENEMANAGER_TEST
#pragma message(">>> SCENEMANAGER main compiling")
#endif

#ifdef RUN_SCENEMANAGER_TEST

class MenuScene final : public IScene {
public:
	MenuScene(SceneController& _controller) : IScene(_controller) {}

	void Update(float dt) override {
		if (IsKeyPressed(KEY_ENTER)) {
			controller.RequestScene("Game");
		}
	}

	void Draw() override {
		ClearBackground(DARKGRAY);
		DrawText("Main Menu", 40, 50, 40, RAYWHITE);
		DrawText("ENTER - Play", 40, 120, 24, LIGHTGRAY);
	}
};

class GameScene final : public IScene {
	Vector2 player{ 400.f, 225.f };
public:
	GameScene(SceneController& _controller) : IScene(_controller) {}

	void OnEnter() override {
		player = Vector2{ 400.f, 225.f };
	}

	void Update(float dt) override {
		if (IsKeyPressed(KEY_ESCAPE)) {
			controller.RequestScene("Menu");
			return;
		}

		const float distance = 240.f * dt;

		if (IsKeyDown(KEY_LEFT))  player.x -= distance;
		if (IsKeyDown(KEY_RIGHT)) player.x += distance;
		if (IsKeyDown(KEY_UP))    player.y -= distance;
		if (IsKeyDown(KEY_DOWN))  player.y += distance;
	}

	void Draw() override {
		ClearBackground(RAYWHITE);
		DrawText("ESC - Menu", 20, 20, 24, DARKGRAY);
		DrawCircleV(player, 20.0f, BLUE);
	}
};

int main() {
	InitWindow(800, 450, "SceneManager + raylib");
	SetTraceLogLevel(LOG_WARNING);

	SetExitKey(KEY_NULL);

	{
		SceneController controller;

		controller.AddScene("Menu", std::make_unique<MenuScene>(controller));
		controller.AddScene("Game", std::make_unique<GameScene>(controller));
		controller.AddTransition("Menu", "Game", { TransitionEffect::WIPE, 0.8f, BLACK });
		controller.AddTransition("Game", "Menu", { TransitionEffect::FADE, 0.5f, BLACK });

		controller.Start("Menu");

		while (!WindowShouldClose()) {

			controller.Update(GetFrameTime());

			BeginDrawing();
			controller.Draw();
			EndDrawing();
		}
		controller.Shutdown();
	}

	CloseWindow();
	return 0;
}

//class SplashScene : public SceneActionNode {
//public:
//	SplashScene(SceneManager& manager) 
//		: SceneActionNode(manager, "Splash") {}
//
//	void OnUpdate() override {
//		ClearBackground(DARKBLUE);
//		DrawText("My Game", 40, 50, 40, RAYWHITE);
//		DrawText("Press ENTER to continue", 40, 120, 24, LIGHTGRAY);
//
//		if (IsKeyPressed(KEY_ENTER)) {
//			sm.current_scene = "Menu";
//		}
//	}
//};
//
//class MenuScene : public SceneActionNode {
//	int frame_count = 0;
//public:
//	MenuScene(SceneManager& manager) 
//		: SceneActionNode(manager, "Menu") {}
//	
//	void OnUpdate() override {
//		ClearBackground(DARKGRAY);
//		DrawText("Main Menu", 40, 50, 40, RAYWHITE);
//		DrawText("ENTER - Play", 40, 120, 24, LIGHTGRAY);
//		DrawText("Q - Quit", 40, 160, 24, LIGHTGRAY);
//
//		if (IsKeyPressed(KEY_Q)) {
//			sm.request_quit = true;
//		}
//		else if (IsKeyPressed(KEY_ENTER)) {
//			sm.current_scene = "Game";
//		}
//	}
//};
//
//class GameScene : public SceneActionNode {
//	Vector2 player{ 400.0f, 225.0f };
//	bool paused = false;
//
//	void UpdateGameplay() {
//		const float distance = 240.0f * GetFrameTime();
//
//		if (IsKeyDown(KEY_LEFT))  player.x -= distance;
//		if (IsKeyDown(KEY_RIGHT)) player.x += distance;
//		if (IsKeyDown(KEY_UP))    player.y -= distance;
//		if (IsKeyDown(KEY_DOWN))  player.y += distance;
//	}
//
//	void DrawGameplay() {
//		ClearBackground(RAYWHITE);
//		DrawText("Arrow keys to move", 20, 20, 24, DARKGRAY);
//		DrawText("ESC - Pause", 20, 55, 24, DARKGRAY);
//		DrawCircleV(player, 20.0f, BLUE);
//	}
//
//	void DrawPauseOverlay() const {
//		// Dim the game underneath.
//		DrawRectangle(
//			0, 0,
//			GetScreenWidth(), GetScreenHeight(),
//			Color{ 0, 0, 0, 170 }
//		);
//
//		const int x = (GetScreenWidth() - 360) / 2;
//		const int y = (GetScreenHeight() - 200) / 2;
//
//		DrawRectangle(x, y, 360, 200, DARKGRAY);
//		DrawText("Paused", x + 30, y + 25, 36, RAYWHITE);
//		DrawText("ESC - Resume", x + 30, y + 90, 24, LIGHTGRAY);
//		DrawText("M - Main Menu", x + 30, y + 130, 24, LIGHTGRAY);
//	}
//
//public:
//	explicit GameScene(SceneManager& manager) 
//		: SceneActionNode(manager, "Game") {}
//
//	void OnEnter() override {
//		SceneActionNode::OnEnter();
//		player = Vector2{ 400.f, 225.f };
//		paused = false;
//	}
//
//	void OnUpdate() override {
//		if (IsKeyPressed(KEY_ESCAPE)) {
//			paused = !paused;
//		}
//
//		if (paused) {
//			if (IsKeyPressed(KEY_M)) {
//				sm.current_scene = "Menu";
//			}
//		} else {
//			UpdateGameplay();
//		}
//
//		DrawGameplay();
//		
//		if (paused) DrawPauseOverlay();
//	}
//};
//
//static void AddScene(
//	SelectorNode& root, 
//	SceneManager& manager, 
//	const char* name, 
//	std::shared_ptr<SceneActionNode> action) {
//
//	auto branch = std::make_shared<SequenceNode>();
//	branch->AddChild(std::make_shared<SceneConditionNode>(manager, name));
//	branch->AddChild(action);
//	root.AddChild(branch);
//}
//
//
//int main() {
//	InitWindow(800, 450, "SceneManager + raylib");
//	SetTraceLogLevel(LOG_WARNING);
//
//	SetExitKey(KEY_NULL);
//
//	{
//		SceneManager manager;
//
//		auto splash = std::make_shared<SplashScene>(manager);
//		auto menu = std::make_shared<MenuScene>(manager);
//		auto game = std::make_shared<GameScene>(manager);
//
//		const std::vector<std::shared_ptr<SceneActionNode>> scenes{
//			splash, menu, game
//		};
//
//		SelectorNode root;
//		AddScene(root, manager, "Splash", splash);
//		AddScene(root, manager, "Menu", menu);
//		AddScene(root, manager, "Game", game);
//
//		while (!WindowShouldClose() && !manager.request_quit) {
//
//			BeginDrawing();
//			ClearBackground(BLACK);
//			const NodeStatus status = root.Tick();
//			EndDrawing();
//
//			// No branch matched the requested scene
//			if (status == NodeStatus::FAILURE) {
//				break;
//			}
//		}
//
//		// Manager doesn't automatically exit on application shutdown
//		for (const auto& scene : scenes) {
//			scene->Stop();
//		}
//	}
//
//
//	CloseWindow();
//	return 0;
//}

#endif