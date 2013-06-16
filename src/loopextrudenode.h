#ifndef LOOPEXTRUDENODE_H_
#define LOOPEXTRUDENODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"

class LoopExtrudeNode : public AbstractPolyNode
{
public:
	LoopExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = 0;
		//open = false;
		//outer = false;
	}
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "loop_extrude"; }

  Filename filename; // only for convenience, not really usable!
	int convexity;
	double fn, fs, fa;
	//bool open,outer;
  Value points, vertices, edges, segments, poly, rect;
	virtual PolySet *evaluate_polyset(class PolySetEvaluator *) const;
};

#endif
