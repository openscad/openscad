#pragma once

#include <vector>
#include <string>
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
        return this->root_string.substr(start, end-start);
    }

    void insert_start(const class AbstractNode &node, long index) {
        // node index is 1-based
        if (this->startcache.size() <= node.index()) {
            this->startcache.push_back(index);
            this->endcache.push_back(-1L);
        } else {
            assert(false && "start index inserted twice");
        }
    }

    void insert_end(const class AbstractNode &node, long index) {
        assert(this->endcache[node.index()-1] == -1L && "end index inserted twice");
        this->endcache[node.index()-1] = index;
    }

    void set_root_string(const std::string &root_str) {
        this->root_string = root_str;
    }

    void clear() {
        this->startcache.clear();
        this->endcache.clear();
        this->root_string = "";
    }

private:
    std::vector<long> startcache;
    std::vector<long> endcache;

    std::string nullvalue;
    std::string root_string;
};
