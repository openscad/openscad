#ifndef STATE_H_
#define STATE_H_

class State
{
public:
  State(const class AbstractNode *parent) 
    : parentnode(parent), isprefix(false), ispostfix(false), numchildren(0) { }
  virtual ~State() {}
  
  void setPrefix(bool on) { this->isprefix = on; }
  void setPostfix(bool on) { this->ispostfix = on; }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const AbstractNode *parent) { this->parentnode = parent; }

  bool isPrefix() const { return this->isprefix; }
  bool isPostfix() const { return this->ispostfix; }
  unsigned int numChildren() const { return this->numchildren; }
  const AbstractNode *parent() const { return this->parentnode; }

private:
  const AbstractNode * parentnode;
  bool isprefix;
  bool ispostfix;
  unsigned int numchildren;
};

#endif
