#ifndef STATE_H_
#define STATE_H_

#include <cstring>

class State
{
public:
  State(const class AbstractNode *parent) 
    : parentnode(parent), isprefix(false), ispostfix(false), numchildren(0) {
		for (int i=0;i<16;i++) this->m[i] = i % 5 == 0 ? 1.0 : 0.0;
		for (int i=0;i<4;i++) this->c[i] = -1.0;
	}
  virtual ~State() {}
  
  void setPrefix(bool on) { this->isprefix = on; }
  void setPostfix(bool on) { this->ispostfix = on; }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const AbstractNode *parent) { this->parentnode = parent; }
	void setMatrix(const double m[16]) { memcpy(this->m, m, 16*sizeof(double)); }
	void setColor(const double c[4]) { memcpy(this->c, c, 4*sizeof(double)); }

  bool isPrefix() const { return this->isprefix; }
  bool isPostfix() const { return this->ispostfix; }
  unsigned int numChildren() const { return this->numchildren; }
  const AbstractNode *parent() const { return this->parentnode; }
	const double *matrix() const { return this->m; }
	const double *color() const { return this->c; }

private:
  const AbstractNode * parentnode;
  bool isprefix;
  bool ispostfix;
  unsigned int numchildren;

	// Transformation matrix and color. FIXME: Generalize such state variables?
	double m[16];
	double c[4];
};

#endif
