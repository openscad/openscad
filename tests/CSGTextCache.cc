#include "CSGTextCache.h"

bool CSGTextCache::contains(const AbstractNode &node) const
{
	return this->cache.contains(this->tree.getString(node));
}

// We cannot return a reference since the [] operator returns a temporary value
string CSGTextCache::operator[](const AbstractNode &node) const
{
	return this->cache[this->tree.getString(node)];
}

void CSGTextCache::insert(const class AbstractNode &node, const string & value)
{
	this->cache.insert(this->tree.getString(node), value);
}

void CSGTextCache::remove(const class AbstractNode &node)
{
	this->cache.remove(this->tree.getString(node));
}

void CSGTextCache::clear()
{
	this->cache.clear();
}
