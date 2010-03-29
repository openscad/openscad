#ifndef NODECACHE_H_
#define NODECACHE_H_

#include <vector>
#include "node.h"

template <class T>
class NodeCache
{
public:
  NodeCache() { }
  virtual ~NodeCache() { }

  const T & operator[](const AbstractNode &node) const {
    if (this->cache.size() > node.index()) return this->cache[node.index()];
    else return nullvalue;
  }

  void insert(const class AbstractNode &node, const T & value) {
    if (this->cache.size() <= node.index()) this->cache.resize(node.index() + 1);
    this->cache[node.index()] = value;
  }

  void remove(const class AbstractNode &node) {
    if (this->cache.size() > node.index()) this->cache[node.index()] = nullvalue;
  }

	void clear() {
		this->cache.clear();
	}

private:
  std::vector<T> cache;
  T nullvalue;
};

#endif
