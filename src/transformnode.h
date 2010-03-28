#ifndef TRANSFORMNODE_H_
#define TRANSFORMNODE_H_

#include "node.h"
#include "visitor.h"
#ifdef ENABLE_CGAL
#  include "cgal.h"
#endif

class TransformNode : public AbstractNode
{
public:
	TransformNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  virtual Response accept(const class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

	double m[20];
#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron renderCSGMesh() const;
#endif
	virtual CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
#ifndef REMOVE_DUMP
	virtual QString dump(QString indent) const;
#endif
};

#endif
