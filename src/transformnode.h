#ifndef TRANSFORMNODE_H_
#define TRANSFORMNODE_H_

#include "node.h"
#include "visitor.h"

class TransformNode : public AbstractNode
{
public:
	TransformNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  virtual Response accept(const class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

	double m[20];
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
};

#endif
