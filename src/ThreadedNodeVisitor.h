#pragma once

#include "NodeVisitor.h"
#include "progress.h"
#include "Tree.h"
#include "CGAL_Nef_polyhedron.h"

class ThreadedNodeVisitor : public NodeVisitor {
  const Tree &_tree;

public:
  ThreadedNodeVisitor(const Tree &tree) : _tree(tree) {}

  Response traverseThreaded(const AbstractNode &node, const class State &state = NodeVisitor::nullstate);

  // Number of threads to spawn for parallel traversal. If 0 (the default), uses
  // the value returned from std::thread::hardware_concurrency().
  static int Parallelism;
};
