#ifndef EXPORTCURVESNODE_H_
#define EXPORTCURVESNODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"

class ExportCurvesNode : public AbstractPolyNode
{
public:
	ExportCurvesNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = 0;
		origin_x = origin_y = scale = 0;
	}
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "export_curves"; }

	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	Filename filename;
	std::string layername;
	virtual PolySet *evaluate_polyset(class PolySetEvaluator *) const;
};

#endif //EXPORTCURVESNODE_H_
