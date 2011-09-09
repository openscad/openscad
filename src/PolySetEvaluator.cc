#include "PolySetEvaluator.h"
#include "printutils.h"
#include "polyset.h"

/*!
	The task of PolySetEvaluator is to create, keep track of and cache PolySet instances.

	All instances of PolySet which are not strictly temporary should be requested through this
	class.
*/

PolySet *PolySetEvaluator::getPolySet(const AbstractNode &node)
{
	const string &cacheid = this->tree.getString(node);
	if (this->cache.contains(cacheid)) return this->cache[cacheid]->ps;

	PolySet *ps = node.evaluate_polyset(this);
	this->cache.insert(cacheid, new cache_entry(ps), ps?ps->polygons.size():0);
	return ps;
}

PolySetEvaluator::cache_entry::cache_entry(PolySet *ps) 
	: ps(ps)
{
	if (print_messages_stack.size() > 0) this->msg = print_messages_stack.last();
}

void PolySetEvaluator::printCache()
{
	PRINTF("PolySets in cache: %d", this->cache.size());
	PRINTF("Polygons in cache: %d", this->cache.totalCost());
}
