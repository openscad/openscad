#include "CSGTextCache.h"

bool CSGTextCache::contains(const AbstractNode &node) const
{
	return this->cache.find(this->tree.getId(node)) != this->cache.end();
}

// We cannot return a reference since the [] operator returns a temporary value
string CSGTextCache::operator[](const AbstractNode &node) const
{
	return this->cache.at(this->tree.getId(node));
}

void CSGTextCache::insert(const class AbstractNode &node, const string & value)
{
	this->cache.insert(std::make_pair(this->tree.getId(node), value));
}

void CSGTextCache::remove(const class AbstractNode &node)
{
	this->cache.erase(this->tree.getId(node));
}

void CSGTextCache::clear()
{
	this->cache.clear();
}
