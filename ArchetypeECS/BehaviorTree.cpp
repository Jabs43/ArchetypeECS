#include "BehaviorTree.h"

void CompositeNode::AddChild(std::shared_ptr<BTNode> child) {
	children.push_back(child);
}

NodeStatus SequenceNode::Tick() {
	for (const auto& child : children) {
		NodeStatus status = child->Tick();
		if (status != NodeStatus::SUCCESS) {
			return status;// Returns early (FAILURE or RUNNING)
		}
	}
	return NodeStatus::SUCCESS;
}

NodeStatus SelectorNode::Tick() {
	for (const auto& child : children) {
		NodeStatus status = child->Tick();
		if (status != NodeStatus::FAILURE) {
			return status;// Returns early (SUCCESS or RUNNING)
		}
	}
	return NodeStatus::FAILURE;
}
