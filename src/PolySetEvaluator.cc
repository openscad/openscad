#include "PolySetCache.h"
#include "PolySetEvaluator.h"
#include "printutils.h"
#include "polyset.h"
#include "Tree.h"

/*!
	The task of PolySetEvaluator is to create, keep track of and cache PolySet instances.

	All instances of PolySet which are not strictly temporary should be
	requested through this class.
*/

/*!
  Factory method returning a PolySet from the given node. If the
  node is already cached, the cached PolySet will be returned
  otherwise a new PolySet will be created from the node. If cache is
  true, the newly created PolySet will be cached.
 */
shared_ptr<PolySet> PolySetEvaluator::getPolySet(const AbstractNode &node, bool cache)
{
	std::string cacheid = this->tree.getIdString(node);

	if (PolySetCache::instance()->contains(cacheid)) {
#ifdef DEBUG
    // For cache debugging
		PRINTB("PolySetCache hit: %s", cacheid.substr(0, 40));
#endif
		return PolySetCache::instance()->get(cacheid);
	}

	shared_ptr<PolySet> ps(node.evaluate_polyset(this));
	if (cache) PolySetCache::instance()->insert(cacheid, ps);
	return ps;
}
