#ifndef PROJECTIONNODE_H_
#define PROJECTIONNODE_H_

#include "node.h"
#include "visitor.h"

class ProjectionNode : public AbstractPolyNode
{
public:
	ProjectionNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		cut_mode = false;
	}
  virtual Response accept(const class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

	int convexity;
	bool cut_mode;
	virtual PolySet *render_polyset(render_mode_e mode) const;
#ifndef REMOVE_DUMP
	virtual QString dump(QString indent) const;
#endif
};

#endif
