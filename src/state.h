#ifndef STATE_H_
#define STATE_H_

#include <cstring>
#include "linalg.h"

class State
{
public:
  State(const class AbstractNode *parent) 
    : parentnode(parent), isprefix(false), ispostfix(false), numchildren(0) {
		m = Transform3d::Identity();
		for (int i=0;i<4;i++) this->c[i] = -1.0;
	}
  virtual ~State() {}
  
  void setPrefix(bool on) { this->isprefix = on; }
  void setPostfix(bool on) { this->ispostfix = on; }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const AbstractNode *parent) { this->parentnode = parent; }
	void setMatrix(const Transform3d &m) { this->m = m; }
	void setColor(const double c[4]) { memcpy(this->c, c, 4*sizeof(double)); }

  bool isPrefix() const { return this->isprefix; }
  bool isPostfix() const { return this->ispostfix; }
  unsigned int numChildren() const { return this->numchildren; }
  const AbstractNode *parent() const { return this->parentnode; }
	const Transform3d &matrix() const { return this->m; }
	const double *color() const { return this->c; }

private:
  const AbstractNode * parentnode;
  bool isprefix;
  bool ispostfix;
  unsigned int numchildren;

	// Transformation matrix and color. FIXME: Generalize such state variables?
	Transform3d m;
	double c[4];
};

#endif
