#ifndef CGALADVNODE_H_
#define CGALADVNODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"
#include "linalg.h"

enum cgaladv_type_e {
	MINKOWSKI,
	GLIDE,
	SUBDIV,
	HULL,
	RESIZE,
	//RUUD
	BOX,
	POSITION
};

class CgaladvNode : public AbstractNode
{
public:
	CgaladvNode(const ModuleInstantiation *mi, cgaladv_type_e type) : AbstractNode(mi), type(type) {
		convexity = 1;
	}
	virtual ~CgaladvNode() { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const;
	PolySet *evaluate_polyset(class PolySetEvaluator *ps) const;

	Value path;
	std::string subdiv_type;
	int convexity, level;
	Vector3d newsize;
	Eigen::Matrix<bool,3,1> autosize;
	cgaladv_type_e type;
	//RUUD
  Value keep,xmin,xmid,xmax,ymin,ymid,ymax,zmin,zmid,zmax;
  Value xadd,yadd,zadd,add,act;


};

#endif
