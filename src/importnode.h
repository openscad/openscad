#ifndef IMPORTNODE_H_
#define IMPORTNODE_H_

#include "node.h"
#include "visitor.h"

enum import_type_e {
	TYPE_STL,
	TYPE_OFF,
	TYPE_DXF
};

class ImportNode : public AbstractPolyNode
{
public:
	ImportNode(const ModuleInstantiation *mi, import_type_e type) : AbstractPolyNode(mi), type(type) { }
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;

	import_type_e type;
	QString filename;
	QString layername;
	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	virtual PolySet *render_polyset(render_mode_e mode) const;
};

#endif
