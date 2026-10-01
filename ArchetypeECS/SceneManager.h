#pragma once
#include "BehaviorTree.h"
#include <cstdio>

struct SceneManager {// Used as a blackboard
	std::string current_scene = "Splash";
	bool request_quit = false;
};

class SceneNode : public BTNode {
protected:
	SceneManager& sm;
public:
	SceneNode(SceneManager& manager) : sm(manager) {}
};

class SceneConditionNode : public SceneNode {
	std::string target_scene;
public:
	SceneConditionNode(SceneManager& manager, std::string scene) : SceneNode(manager), target_scene(scene) {}

	NodeStatus Tick() override {
		return (sm.current_scene == target_scene) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
	}
};

class SceneActionNode : public SceneNode {
protected:
	std::string name;
	bool initialized = false;
public:
	SceneActionNode(SceneManager& manager, std::string scene_name) : SceneNode(manager), name(scene_name) {}

	virtual void OnEnter() { printf("[Entering Scene] %s\n", name.c_str()); }
	virtual void OnUpdate() = 0;
	virtual void OnExit() { printf("[Exiting Scene] %s\n", name.c_str()); initialized = false; }

	void Stop() {
		if (!initialized) return;
		OnExit();
		initialized = false;
	}

	NodeStatus Tick() override {
		if (!initialized) {
			OnEnter();
			initialized = true;
		}

		OnUpdate();

		if (sm.current_scene != name) {
			Stop();
			return NodeStatus::SUCCESS;// Transition handled succefully
		}

		return NodeStatus::RUNNING;
	}
};