#pragma once

#include <vector>
#include <string>
#include <istream>
#include "node.h"
#include "memory.h"
#include "assert.h"

/*!
	Caches string values per node based on the node.index().
	The node index guaranteed to be unique per node tree since the index is reset
	every time a new tree is generated.
*/

typedef std::pair<std::streampos, std::streampos> IndexPair;

class NodeCache
{
public:
  NodeCache() { }
  virtual ~NodeCache() { }

	bool contains(const AbstractNode &node) const {
        return this->root_stream.get();
	}

  const std::string operator[](const AbstractNode &node) const {
    assert(this->cache.size() > node.index());
    const IndexPair &p = this->cache[node.index()];
    this->root_stream->seekg(p.first);
    this->root_stream->read(p.second - p.first);
    
  }

  void insert(const class AbstractNode &node, std::stringstream &dump) {
    if (this->cache.size() <= node.index()) this->cache.resize(node.index() + 1);
    const IndexPair &p = this->cache[node.index()];
    if (p.second < 0) {
        this->cache[node.index()] = std::make_pair(p.first, dump.tellp()));
    } else {
        this->cache[node.index()] = std::make_pair(dump.tellp(), -1);
    }
  }

  void set_root_stream(std::stringstream& dump) {
      this->root_stream.reset(dump);
  }

/*
  void remove(const class AbstractNode &node) {
     //;
  }
*/

	void clear() {
		this->cache.clear();
        this->root_string.reset();
	}

private:
  std::vector<IndexPair> cache;
  std::string nullvalue;
  shared_ptr<std::stringstream> root_stream;
};
