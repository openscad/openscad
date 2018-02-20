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
        return this->startcache.size() > node.index()-1 && 
            this->endcache.size() > node.index()-1 &&
            this->endcache[node.index()-1] >= 0;
	}

  const std::string operator[](const AbstractNode &node) const {
    assert(contains(node));
    long start = this->startcache[node.index()-1];
    long end = this->endcache[node.index()-1];
    std::stringstream &root_stream = *this->root_stream;
    std::stringstream o;
    root_stream.seekg(start);
    std::copy_n(std::istream_iterator<char>(root_stream), (end - start), std::ostream_iterator<char>(o));
    return o.str();
  }

  void insert_start(const class AbstractNode &node) {
      // node index is 1-based
  if (this->startcache.size() <= node.index()) {
        this->startcache.push_back(this->root_stream->tellp());
        this->endcache.push_back(-1L);
    } else {
        assert(false && "start index inserted twice");
    }
  }

  void insert_end(const class AbstractNode &node) {
      assert(this->endcache[node.index()-1] == -1L && "end index inserted twice");
      this->endcache[node.index()-1] = this->root_stream->tellp();
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
        this->startcache.clear();
        this->endcache.clear();
        this->root_stream.reset();
    }

private:
  std::vector<long> startcache;
  std::vector<long> endcache;
  
  std::string nullvalue;
  shared_ptr<std::stringstream> root_stream;
};
