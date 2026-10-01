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
	void AddChild(std::shared_ptr<BTNode> child);
};

class SequenceNode : public CompositeNode {
public:
	NodeStatus Tick() override;
};

class SelectorNode : public CompositeNode {
public:
	NodeStatus Tick() override;
};

