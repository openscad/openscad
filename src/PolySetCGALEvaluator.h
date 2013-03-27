#ifndef POLYSETCGALEVALUATOR_H_
#define POLYSETCGALEVALUATOR_H_

#include "PolySetEvaluator.h"

/*!
	This is a Geometry evaluator which uses the CGALEvaluator to support building
	geometrys.
*/
class PolySetCGALEvaluator : public PolySetEvaluator
{
public:
	PolySetCGALEvaluator(class CGALEvaluator &cgalevaluator);
	virtual ~PolySetCGALEvaluator() { }
	virtual Geometry *evaluateGeometry(const ProjectionNode &node);
	virtual Geometry *evaluateGeometry(const LinearExtrudeNode &node);
	virtual Geometry *evaluateGeometry(const RotateExtrudeNode &node);
	virtual Geometry *evaluateGeometry(const CgaladvNode &node);
	virtual Geometry *evaluateGeometry(const RenderNode &node);
	bool debug;
protected:
	Geometry *extrudeDxfData(const LinearExtrudeNode &node, class DxfData &dxf);
	Geometry *rotateDxfData(const RotateExtrudeNode &node, class DxfData &dxf);

	CGALEvaluator &cgalevaluator;
};

#endif
