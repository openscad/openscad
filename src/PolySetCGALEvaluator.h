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
	virtual PolySet *evaluatePolySet(const DxfLinearExtrudeNode &node);
	virtual PolySet *evaluatePolySet(const DxfRotateExtrudeNode &node);
	virtual PolySet *evaluatePolySet(const CgaladvNode &node);
	virtual PolySet *evaluatePolySet(const RenderNode &node);

protected:
	PolySet *extrudeDxfData(const DxfLinearExtrudeNode &node, class DxfData &dxf);
	PolySet *rotateDxfData(const DxfRotateExtrudeNode &node, class DxfData &dxf);

	CGALEvaluator &cgalevaluator;
};

#endif
