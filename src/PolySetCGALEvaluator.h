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
	PolySetCGALEvaluator(class CGALEvaluator &CGALEvaluator) : 
		PolySetEvaluator(), cgalevaluator(CGALEvaluator) { }
	virtual ~PolySetCGALEvaluator() { }
	virtual PolySet *evaluatePolySet(const ProjectionNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *evaluatePolySet(const DxfLinearExtrudeNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *evaluatePolySet(const DxfRotateExtrudeNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *evaluatePolySet(const CgaladvNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *evaluatePolySet(const RenderNode &node, AbstractPolyNode::render_mode_e);

protected:
	PolySet *extrudeDxfData(const DxfLinearExtrudeNode &node, class DxfData &dxf);
	PolySet *rotateDxfData(const DxfRotateExtrudeNode &node, class DxfData &dxf);

	CGALEvaluator &cgalevaluator;
};

#endif
