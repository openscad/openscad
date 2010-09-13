#ifndef STATE_H_
#define STATE_H_

class State
{
public:
  State(const class AbstractNode *parent) 
    : parentnode(parent), isprefix(false), ispostfix(false), numchildren(0) {
		for (int i=0;i<16;i++) this->m[i] = i % 5 == 0 ? 1.0 : 0.0;
		for (int i=16;i<20;i++) this->m[i] = -1.0;
	}
  virtual ~State() {}
  
  void setPrefix(bool on) { this->isprefix = on; }
  void setPostfix(bool on) { this->ispostfix = on; }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const AbstractNode *parent) { this->parentnode = parent; }
	void setMatrix(const double m[20]) { memcpy(this->m, m, sizeof(m)); }

  bool isPrefix() const { return this->isprefix; }
  bool isPostfix() const { return this->ispostfix; }
  unsigned int numChildren() const { return this->numchildren; }
  const AbstractNode *parent() const { return this->parentnode; }
	const double *matrix() const { return this->m; }

private:
  const AbstractNode * parentnode;
  bool isprefix;
  bool ispostfix;
  unsigned int numchildren;

	// Transformation matrix incl. color. FIXME: Generalize such state variables?
	double m[20];
};

#endif
