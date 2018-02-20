#pragma once

#include <vector>
#include <string>
#include <iterator>
#include <sstream>
#include <algorithm>
#include "node.h"
#include "memory.h"
#include "assert.h"

/*!
	Caches string values per node based on the node.index().
	The node index guaranteed to be unique per node tree since the index is reset
	every time a new tree is generated.
*/

typedef std::pair<long, long> IndexPair;

class NodeCache
{
public:
  NodeCache() { }
  virtual ~NodeCache() { }

	bool contains(const AbstractNode &node) const {
        return this->cache.size() > node.index();
	}

  const std::string operator[](const AbstractNode &node) const {
    assert(this->cache.size() > node.index());
    std::stringstream &root_stream = *this->root_stream;
    std::stringstream o;
    const IndexPair &p = this->cache[node.index()];
    root_stream.seekg(p.first);
    std::copy_n(std::istream_iterator<char>(root_stream), (p.second - p.first), std::ostream_iterator<char>(o));
    return o.str();
  }

  void insert(const class AbstractNode &node) {
    if (this->cache.size() <= node.index()) {
        this->cache.push_back(std::make_pair(this->root_stream->tellp(), -1));
    } else {
        this->cache[node.index()] = std::make_pair(this->cache[node.index()].first, this->root_stream->tellp());
    }
        
  }

  void set_root_stream(std::shared_ptr<std::stringstream> dump) {
      this->root_stream = dump;
  }

/*
  void remove(const class AbstractNode &node) {
     //;
  }
*/

	void clear() {
		this->cache.clear();
        this->root_stream.reset();
	}

private:
  std::vector<IndexPair> cache;
  std::string nullvalue;
  shared_ptr<std::stringstream> root_stream;
};
