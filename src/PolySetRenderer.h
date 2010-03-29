#ifndef POLYSETRENDERER_H_
#define POLYSETRENDERER_H_

#include "node.h"

class PolySetRenderer
{
public:
	enum RenderMode { RENDER_CGAL, RENDER_OPENCSG };
	PolySetRenderer() {}
	virtual ~PolySetRenderer() {}

	virtual PolySet *renderPolySet(const class ProjectionNode &node, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *renderPolySet(const class DxfLinearExtrudeNode &node, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *renderPolySet(const class DxfRotateExtrudeNode &node, AbstractPolyNode::render_mode_e) = 0;

	static PolySetRenderer *renderer() { return global_renderer; }
	static void setRenderer(PolySetRenderer *r) { global_renderer = r; }
private:
	static PolySetRenderer *global_renderer;
};

#endif
