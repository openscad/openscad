#include "CSGTextCache.h"

bool CSGTextCache::contains(const AbstractNode &node) const
{
	return this->cache.find(this->tree.getString(node)) != this->cache.end();
}

// We cannot return a reference since the [] operator returns a temporary value
string CSGTextCache::operator[](const AbstractNode &node) const
{
	return this->cache.at(this->tree.getString(node));
}

void CSGTextCache::insert(const class AbstractNode &node, const string & value)
{
	this->cache.insert(std::make_pair(this->tree.getString(node), value));
}

void CSGTextCache::remove(const class AbstractNode &node)
{
	this->cache.erase(this->tree.getString(node));
}

void CSGTextCache::clear()
{
	this->cache.clear();
}
