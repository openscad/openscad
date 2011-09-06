#ifndef POLYSETEVALUATOR_H_
#define POLYSETEVALUATOR_H_

#include "myqhash.h"
#include "node.h"
#include <QCache>

class PolySetEvaluator
{
public:
	enum EvaluateMode { EVALUATE_CGAL, EVALUATE_OPENCSG };
	PolySetEvaluator() : cache(100) {}

	virtual ~PolySetEvaluator() {}

	virtual PolySet *evaluatePolySet(const class ProjectionNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *evaluatePolySet(const class DxfLinearExtrudeNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *evaluatePolySet(const class DxfRotateExtrudeNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *evaluatePolySet(const class CgaladvNode &, AbstractPolyNode::render_mode_e) = 0;
	virtual PolySet *evaluatePolySet(const class RenderNode &, AbstractPolyNode::render_mode_e) = 0;

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
	static PolySetEvaluator *global_evaluator;
};

#endif
