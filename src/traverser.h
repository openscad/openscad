#pragma once

#include "node.h"

class Traverser
{
public:
  enum TraversalType {PREFIX, POSTFIX, PRE_AND_POSTFIX};

  Traverser(BaseVisitor &visitor, const AbstractNode &root, TraversalType travtype)
		: visitor(visitor), root(root), traversaltype(travtype) {
  }
  virtual ~Traverser() { }
  
  void execute();
  // FIXME: reverse parameters
  Response traverse(const AbstractNode &node, const class State &state);
private:

  BaseVisitor &visitor;
  const AbstractNode &root;
  TraversalType traversaltype;
};
