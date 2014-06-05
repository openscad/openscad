#pragma once

#include <vector>
#include <string>
#include "node.h"

/*!
	Caches string values per node based on the node.index().
	The node index guaranteed to be unique per node tree since the index is reset
	every time a new tree is generated.
*/
class NodeCache
{
public:
  NodeCache() { }
  virtual ~NodeCache() { }

	bool contains(const AbstractNode &node) const {
 		return !(*this)[node].empty();
	}

  /*! Returns a reference to the cached string copy. NB! don't rely on
	 *  this reference to be valid for long - if the cache is resized
	 *  internally, existing values are lost.  */
  const std::string & operator[](const AbstractNode &node) const {
    if (this->cache.size() > node.index()) return this->cache[node.index()];
    else return this->nullvalue;
  }

  /*! Returns a reference to the cached string copy. NB! don't rely on
	 *  this reference to be valid for long - if the cache is resized
	 *  internally, existing values are lost. */
  const std::string &insert(const class AbstractNode &node, const std::string & value) {
    if (this->cache.size() <= node.index()) this->cache.resize(node.index() + 1);
    return this->cache[node.index()] = value;
  }

  void remove(const class AbstractNode &node) {
    if (this->cache.size() > node.index()) this->cache[node.index()] = std::string();
  }

	void clear() {
		this->cache.clear();
	}

private:
  std::vector<std::string> cache;
	std::string nullvalue;
};
