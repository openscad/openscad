#include "GeometryCache.h"
#include "PolySetEvaluator.h"
#include "printutils.h"
#include "polyset.h"
#include "Tree.h"

/*!
	The task of PolySetEvaluator is to create, keep track of and cache Geometry instances.

	All instances of Geometry which are not strictly temporary should be
	requested through this class.
*/

/*!
  Factory method returning a Geometry from the given node. If the
  node is already cached, the cached Geometry will be returned
  otherwise a new Geometry will be created from the node. If cache is
  true, the newly created Geometry will be cached.
 */
shared_ptr<Geometry> PolySetEvaluator::getGeometry(const AbstractNode &node, bool cache)
{
	std::string cacheid = this->tree.getIdString(node);

	if (GeometryCache::instance()->contains(cacheid)) {
#ifdef DEBUG
    // For cache debugging
		PRINTB("GeometryCache hit: %s", cacheid.substr(0, 40));
#endif
		return GeometryCache::instance()->get(cacheid);
	}

	shared_ptr<Geometry> geom(node.evaluate_geometry(this));
	if (cache) GeometryCache::instance()->insert(cacheid, geom);
	return geom;
}
