#pragma once

#include <cstring>
#include "linalg.h"

#define FLAG(var, flag, on) on ? (var |= flag) : (var &= ~flag)

class State
{
public:
	State(const class AbstractNode *parent)
		: flags(NONE), parentnode(parent), numchildren(0) {
		this->matrix_ = Transform3d::Identity();
		this->color_.fill(-1.0f);
	}
	virtual ~State() {}

	void setPrefix(bool on) { FLAG(this->flags, PREFIX, on); }
	void setPostfix(bool on) { FLAG(this->flags, POSTFIX, on); }
	void setHighlight(bool on) { FLAG(this->flags, HIGHLIGHT, on); }
	void setBackground(bool on) { FLAG(this->flags, BACKGROUND, on); }
	void setNumChildren(unsigned int numc) { this->numchildren = numc; }
	void setParent(const AbstractNode *parent) { this->parentnode = parent; }
	void setMatrix(const Transform3d &m) { this->matrix_ = m; }
	void setColor(const Color4f &c) { this->color_ = c; }
	void setPreferNef(bool on) { FLAG(this->flags, PREFERNEF, on); }
	bool preferNef() const { return this->flags & PREFERNEF; }

	bool isPrefix() const { return this->flags & PREFIX; }
	bool isPostfix() const { return this->flags & POSTFIX; }
	bool isHighlight() const { return this->flags & HIGHLIGHT; }
	bool isBackground() const { return this->flags & BACKGROUND; }
	unsigned int numChildren() const { return this->numchildren; }
	const AbstractNode *parent() const { return this->parentnode; }
	const Transform3d &matrix() const { return this->matrix_; }
	const Color4f &color() const { return this->color_; }

private:
	enum StateFlags {
		NONE       = 0x00,
		PREFIX     = 0x01,
		POSTFIX    = 0x02,
		PREFERNEF  = 0x04,
		HIGHLIGHT  = 0x08,
		BACKGROUND = 0x10
	};

	unsigned int flags;
	const AbstractNode *parentnode;
	unsigned int numchildren;

	// Transformation matrix and color. FIXME: Generalize such state variables?
	Transform3d matrix_;
	Color4f color_;
};
