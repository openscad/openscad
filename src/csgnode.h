#ifndef CSGNODE_H_
#define CSGNODE_H_

#include "node.h"
#include "visitor.h"
#ifdef ENABLE_CGAL
#  include "cgal.h"
#endif

enum csg_type_e {
	CSG_TYPE_UNION,
	CSG_TYPE_DIFFERENCE,
	CSG_TYPE_INTERSECTION
};

class CsgNode : public AbstractNode
{
public:
	csg_type_e type;
	CsgNode(const ModuleInstantiation *mi, csg_type_e type) : AbstractNode(mi), type(type) { }
  virtual Response accept(const class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

#ifdef ENABLE_CGAL
	virtual CGAL_Nef_polyhedron renderCSGMesh() const;
#endif
	CSGTerm *render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const;
	virtual QString dump(QString indent) const;
};

#endif
