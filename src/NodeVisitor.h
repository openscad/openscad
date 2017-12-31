#pragma once

#include "BaseVisitable.h"
#include "node.h"
#include "state.h"

class NodeVisitor :
	public BaseVisitor,
	public Visitor<class AbstractNode>,
	public Visitor<class AbstractIntersectionNode>,
	public Visitor<class AbstractPolyNode>,
	public Visitor<class GroupNode>,
	public Visitor<class RootNode>,
	public Visitor<class LeafNode>,
	public Visitor<class CgaladvNode>,
	public Visitor<class CsgOpNode>,
	public Visitor<class LinearExtrudeNode>,
	public Visitor<class RotateExtrudeNode>,
	public Visitor<class ImportNode>,
	public Visitor<class PrimitiveNode>,
	public Visitor<class TextNode>,
	public Visitor<class ProjectionNode>,
	public Visitor<class RenderNode>,
	public Visitor<class SurfaceNode>,
	public Visitor<class TransformNode>,
	public Visitor<class ColorNode>,
	public Visitor<class OffsetNode>
{
public:
  NodeVisitor() {}
  ~NodeVisitor() {}
  
	Response traverse(const AbstractNode &node, const class State &state = NodeVisitor::nullstate);

  Response visit(class State &state, const class AbstractNode &node) override = 0;
  Response visit(class State &state, const class AbstractIntersectionNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class AbstractPolyNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class GroupNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const RootNode &node) override {
		return visit(state, (const class GroupNode &)node);
	}
  Response visit(class State &state, const class LeafNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  Response visit(class State &state, const class CgaladvNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class CsgOpNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class LinearExtrudeNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  Response visit(class State &state, const class RotateExtrudeNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  Response visit(class State &state, const class ImportNode &node) override {
		return visit(state, (const class LeafNode &)node);
	}
  Response visit(class State &state, const class PrimitiveNode &node) override {
		return visit(state, (const class LeafNode &)node);
	}
  Response visit(class State &state, const class TextNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  Response visit(class State &state, const class ProjectionNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  Response visit(class State &state, const class RenderNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class SurfaceNode &node) override {
		return visit(state, (const class LeafNode &)node);
	}
  Response visit(class State &state, const class TransformNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class ColorNode &node) override {
		return visit(state, (const class AbstractNode &)node);
	}
  Response visit(class State &state, const class OffsetNode &node) override {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	// Add visit() methods for new visitable subtypes of AbstractNode here

private:
	static State nullstate;
};
