#pragma once
#include "raylib.h"
#include "SceneManager.h"


#ifdef RUN_SCENEMANAGER_TEST
#pragma message(">>> SCENEMANAGER main compiling")
#endif

#ifdef RUN_SCENEMANAGER_TEST

class MainMenuScene : public SceneActionNode {
	int frame_count = 0;
public:
	MainMenuScene(SceneManager& manager) : SceneActionNode(manager, "Menu") {}
	
	void OnUpdate() override {
		printf("MainMenu Update...\n");
		frame_count++;
		if (frame_count >= 2) {
			sm.current_scene = "Gameplay";
		}
	}
};

class GamePlayScene : public SceneActionNode {
	int loops = 0;
public:
	GamePlayScene(SceneManager& manager) : SceneActionNode(manager, "Gameplay") {}

	void OnUpdate() override {
		printf("Playing Game (Loop %d )...\n", ++loops);
		if (loops >= 3) {
			printf("Player closed the game.\n");
			sm.request_quit = true;
		}
	}
};

class SplashScene : public SceneActionNode {
public:
	SplashScene(SceneManager& manager) : SceneActionNode(manager, "Splash") {}

	void OnUpdate() override {
		printf("Showing Splash Screen Logo...\n");
		sm.current_scene = "Menu";// Auto transition next frame
	}
};


int main() {
	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(64, 64, "bench");
	SetTraceLogLevel(LOG_WARNING);

	SceneManager manager;

	auto root = std::make_shared<SelectorNode>();
	
	// Branch A:
	auto splash_seq = std::make_shared<SequenceNode>();
	splash_seq->AddChild(std::make_shared<SceneConditionNode>(manager, "Splash"));
	splash_seq->AddChild(std::make_shared<SplashScene>(manager));
	root->AddChild(splash_seq);

	// Branch B:
	auto menu_seq = std::make_shared<SequenceNode>();
	menu_seq->AddChild(std::make_shared<SceneConditionNode>(manager, "Menu"));
	menu_seq->AddChild(std::make_shared<MainMenuScene>(manager));
	root->AddChild(menu_seq);

	// Branch C:
	auto gameplay_seq = std::make_shared<SequenceNode>();
	gameplay_seq->AddChild(std::make_shared<SceneConditionNode>(manager, "Gameplay"));
	gameplay_seq->AddChild(std::make_shared<GamePlayScene>(manager));
	root->AddChild(gameplay_seq);

	printf("--- Starting BT Engine Loop ---\n");

	while (!manager.request_quit) {
		root->Tick();
	}

	printf("Engine Shutdown Succefully.\n");

	std::getchar();   // keep the console open until you press Enter
	CloseWindow();
	return 0;
}

#endif