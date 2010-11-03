#ifndef POLYSETCGALRENDERER_H_
#define POLYSETCGALRENDERER_H_

#include "PolySetRenderer.h"

/*!
	This is a PolySet renderer which uses the CGALRenderer to support building
	polysets.
*/
class PolySetCGALRenderer : public PolySetRenderer
{
public:
	PolySetCGALRenderer(class CGALRenderer &cgalrenderer) : 
		PolySetRenderer(), cgalrenderer(cgalrenderer) { }
	virtual ~PolySetCGALRenderer() { }
	virtual PolySet *renderPolySet(const ProjectionNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *renderPolySet(const DxfLinearExtrudeNode &node, AbstractPolyNode::render_mode_e);
	virtual PolySet *renderPolySet(const DxfRotateExtrudeNode &node, AbstractPolyNode::render_mode_e);

private:
	CGALRenderer &cgalrenderer;
};

#endif
