#ifndef POLYSETEVALUATOR_H_
#define POLYSETEVALUATOR_H_

#include "myqhash.h"
#include "node.h"
#include "Tree.h"
#include <QCache>

class PolySetEvaluator
{
public:
	PolySetEvaluator(const Tree &tree) : tree(tree) {}
	virtual ~PolySetEvaluator() {}

	const Tree &getTree() const { return this->tree; }

	virtual PolySet *getPolySet(const class AbstractNode &);

	virtual PolySet *evaluatePolySet(const class ProjectionNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class DxfLinearExtrudeNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class DxfRotateExtrudeNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class CgaladvNode &) { return NULL; }
	virtual PolySet *evaluatePolySet(const class RenderNode &) { return NULL; }

	void clearCache() {
		cache.clear();
	}
	void printCache();
protected:

	struct cache_entry {
		class PolySet *ps;
		QString msg;
		cache_entry(PolySet *ps);
		~cache_entry() { }
	};

	static QCache<std::string, cache_entry> cache;

private:
	const Tree &tree;
};

#endif
