#ifndef POLYSETCGALEVALUATOR_H_
#define POLYSETCGALEVALUATOR_H_

#include "PolySetEvaluator.h"

/*!
	This is a PolySet evaluator which uses the CGALEvaluator to support building
	polysets.
*/
class PolySetCGALEvaluator : public PolySetEvaluator
{
public:
	PolySetCGALEvaluator(class CGALEvaluator &cgalevaluator);
	virtual ~PolySetCGALEvaluator() { }
	virtual PolySet *evaluatePolySet(const ProjectionNode &node);
	virtual PolySet *evaluatePolySet(const LinearExtrudeNode &node);
	virtual PolySet *evaluatePolySet(const RotateExtrudeNode &node);
	virtual PolySet *evaluatePolySet(const CgaladvNode &node);
	virtual PolySet *evaluatePolySet(const RenderNode &node);
	bool debug;
protected:
	PolySet *extrudeDxfData(const LinearExtrudeNode &node, class DxfData &dxf);
	PolySet *rotateDxfData(const RotateExtrudeNode &node, class DxfData &dxf);

	CGALEvaluator &cgalevaluator;
};

#endif
