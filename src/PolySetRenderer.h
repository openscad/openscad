#ifndef POLYSETRENDERER_H_
#define POLYSETRENDERER_H_

#include "myqhash.h"
#include "node.h"
#include <QCache>

class PolySetRenderer
{
public:
	enum RenderMode { RENDER_CGAL, RENDER_OPENCSG };
	PolySetRenderer() : cache(100) {}

	virtual ~PolySetRenderer() {}

	virtual PolySet *renderPolySet(const class ProjectionNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *renderPolySet(const class DxfLinearExtrudeNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *renderPolySet(const class DxfRotateExtrudeNode &, AbstractPolyNode::render_mode_e) = 0;

	void clearCache() {
		this->cache.clear();
	}

protected:

	struct cache_entry {
		class PolySet *ps;
		QString msg;
		cache_entry(PolySet *ps);
		~cache_entry();
	};

	QCache<std::string, cache_entry> cache;

private:
	static PolySetRenderer *global_renderer;
};

#endif
