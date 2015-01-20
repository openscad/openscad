#pragma once

#include <cstring>
#include "linalg.h"

class State
{
public:
  State(const class AbstractNode *parent) 
    : parentnode(parent), isprefix(false), ispostfix(false), numchildren(0), prefernef(false) {
		this->matrix_ = Transform3d::Identity();
		this->color_.fill(-1.0f);
	}
  virtual ~State() {}
  
  void setPrefix(bool on) { this->isprefix = on; }
  void setPostfix(bool on) { this->ispostfix = on; }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const AbstractNode *parent) { this->parentnode = parent; }
	void setMatrix(const Transform3d &m) { this->matrix_ = m; }
	void setColor(const Color4f &c) { this->color_ = c; }
	void setPreferNef(bool on) { this->prefernef = on; }
	bool preferNef() const { return this->prefernef; }

  bool isPrefix() const { return this->isprefix; }
  bool isPostfix() const { return this->ispostfix; }
  unsigned int numChildren() const { return this->numchildren; }
  const AbstractNode *parent() const { return this->parentnode; }
	const Transform3d &matrix() const { return this->matrix_; }
	const Color4f &color() const { return this->color_; }

private:
  const AbstractNode * parentnode;
  bool isprefix;
  bool ispostfix;
  unsigned int numchildren;

	bool prefernef;
	// Transformation matrix and color. FIXME: Generalize such state variables?
	Transform3d matrix_;
	Color4f color_;
};
