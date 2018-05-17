#pragma once

#include "NodeVisitor.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"

class ThreadedNodeVisitor : public NodeVisitor {
  // TODO: is this param needed?
  bool _threaded; // indicates this visitor should actually use threads

  const Tree &_tree;

protected:
  virtual void smartCacheInsert(const AbstractNode &node, const shared_ptr<const Geometry> &geom);

public:
  ThreadedNodeVisitor(const Tree &tree, bool threaded = false) : _threaded(threaded), _tree(tree) {}

  Response traverseThreaded(const AbstractNode &node, const class State &state = NodeVisitor::nullstate);
};
