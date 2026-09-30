#pragma once
#include <memory>
#include <string>
#include <vector>

enum class NodeStatus { SUCCESS, FAILURE, RUNNING };

class BTNode {
public:
	virtual ~BTNode() = default;
	virtual NodeStatus Tick() = 0;
};

class CompositeNode : public BTNode {
protected:
	std::vector<std::shared_ptr<BTNode>> children;
public:
	void AddChild(std::shared_ptr<BTNode> child) {
		children.push_back(child);
	}
};

class SequenceNode : public CompositeNode {
public:
	NodeStatus Tick() override {
		for (const auto& child : children) {
			NodeStatus status = child->Tick();
			if (status != NodeStatus::SUCCESS) {
				return status;// Returns early (FAILURE or RUNNING)
			}
		}
		return NodeStatus::SUCCESS;
	}
};

class SelectorNode : public CompositeNode {
public:
	NodeStatus Tick() override {
		for (const auto& child : children) {
			NodeStatus status = child->Tick();
			if (status != NodeStatus::FAILURE) {
				return status;// Returns early (SUCCESS or RUNNING)
			}
		}
		return NodeStatus::FAILURE;
	}
};

