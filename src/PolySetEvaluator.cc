#include "PolySetEvaluator.h"
#include "printutils.h"
#include "polyset.h"

/*!
	The task of PolySetEvaluator is to create, keep track of and cache PolySet instances.

	All instances of PolySet which are not strictly temporary should be requested through this
	class.
*/

QCache<std::string, PolySetEvaluator::cache_entry> PolySetEvaluator::cache(100000);

/*!
  Factory method returning a PolySet from the given node. If the
  node is already cached, the cached PolySet will be returned
  otherwise a new PolySet will be created from the node. If cache is
  true, the newly created PolySet will be cached.
 */
shared_ptr<PolySet> PolySetEvaluator::getPolySet(const AbstractNode &node, bool cache)
{
	std::string cacheid = this->tree.getString(node);
	if (this->cache.contains(cacheid)) {
		PRINTF("Cache hit: %s", cacheid.substr(0, 40).c_str());
		return this->cache[cacheid]->ps;
	}

	shared_ptr<PolySet> ps(node.evaluate_polyset(this));
	if (cache) this->cache.insert(cacheid, new cache_entry(ps), ps?ps->polygons.size():0);
	return ps;
}

PolySetEvaluator::cache_entry::cache_entry(const shared_ptr<PolySet> &ps) : ps(ps)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.last();
}

void PolySetEvaluator::printCache()
{
	PRINTF("PolySets in cache: %d", cache.size());
	PRINTF("Polygons in cache: %d", cache.totalCost());
}
