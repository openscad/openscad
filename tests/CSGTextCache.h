#ifndef CSGTEXTCACHE_H_
#define CSGTEXTCACHE_H_

#include "Tree.h"
#include <string>
#include <functional>
#include <boost/unordered_map.hpp>

using std::string;

class CSGTextCache
{
public:
	CSGTextCache(const Tree &tree) : tree(tree) {}
	~CSGTextCache() {}

	bool contains(const AbstractNode &node) const;
	string operator[](const AbstractNode &node) const;
	void insert(const class AbstractNode &node, const string& value);
	void remove(const class AbstractNode &node);
	void clear();

private:
	boost::unordered_map<size_t, string> cache;
	const Tree &tree;
};

#endif
