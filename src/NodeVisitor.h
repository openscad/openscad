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
	virtual ~NodeVisitor() {}

	Response traverse(const AbstractNode &node, const class State &state = NodeVisitor::nullstate);

	virtual Response visit(class State &state, const class AbstractNode &node) = 0;
	virtual Response visit(class State &state, const class AbstractIntersectionNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class AbstractPolyNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class GroupNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const RootNode &node) {
		return visit(state, (const class GroupNode &)node);
	}
	virtual Response visit(class State &state, const class LeafNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	virtual Response visit(class State &state, const class CgaladvNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class CsgOpNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class LinearExtrudeNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	virtual Response visit(class State &state, const class RotateExtrudeNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	virtual Response visit(class State &state, const class ImportNode &node) {
		return visit(state, (const class LeafNode &)node);
	}
	virtual Response visit(class State &state, const class PrimitiveNode &node) {
		return visit(state, (const class LeafNode &)node);
	}
	virtual Response visit(class State &state, const class TextNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	virtual Response visit(class State &state, const class ProjectionNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	virtual Response visit(class State &state, const class RenderNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class SurfaceNode &node) {
		return visit(state, (const class LeafNode &)node);
	}
	virtual Response visit(class State &state, const class TransformNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class ColorNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
	virtual Response visit(class State &state, const class OffsetNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
	// Add visit() methods for new visitable subtypes of AbstractNode here

private:
	static State nullstate;
};
