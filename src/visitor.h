#pragma once

#include "traverser.h"

class Visitor
{
public:
  Visitor() {}
  virtual ~Visitor() {}
  
  virtual Response visit(class State &state, const class AbstractNode &node) = 0;
  virtual Response visit(class State &state, const class AbstractIntersectionNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
  virtual Response visit(class State &state, const class AbstractPolyNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
  virtual Response visit(class State &state, const class LeafNode &node) {
		return visit(state, (const class AbstractPolyNode &)node);
	}
  virtual Response visit(class State &state, const class CgaladvNode &node) {
		return visit(state, (const class AbstractNode &)node);
	}
  virtual Response visit(class State &state, const class CsgNode &node) {
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
};
