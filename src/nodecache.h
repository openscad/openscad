#pragma once

#include <string>
#include <map>
#include "node.h"
#include "memory.h"
#include "assert.h"
#include "printutils.h"

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
    virtual ~NodeCache() { cache.clear(); }

    bool contains(const AbstractNode &node) const {
        auto i = node.index();
        auto result = cache.find(i); 
        return result != cache.end() && result->second.second >= 0L;
    }

    const std::string operator[](const AbstractNode &node) const {
        auto i = node.index();
        auto result = cache.find(i);
        assert(result != cache.end() && "nodecache miss");
        auto start = result->second.first;
        auto end = result->second.second;
        return root_string.substr(start, end-start);
    }

    void insert_start(const class AbstractNode &node, const long strindex) {
        auto i = node.index();
        assert(cache.find(i) == cache.end() && "start index inserted twice");
        cache[i] = std::make_pair(strindex, -1L);
    }

    void insert_end(const class AbstractNode &node, const long strindex) {
        auto i = node.index();
        auto result = cache.find(i);
        assert(result != cache.end() && "end index inserted before start");
        auto indexpair = result->second;
        assert(indexpair.second == -1L && "end index inserted twice");
        cache[i] = std::make_pair(indexpair.first, strindex);
        PRINTDB("NodeCache Insert nodecache[%i] = [%d:%d]", i % indexpair.first % indexpair.second );
    }

    void set_root_string(const std::string &root_str) {
        root_string = root_str;
    }

    void clear() {
        cache.clear();
        root_string = "";
    }

private:
    std::map<size_t, std::pair<long,long>> cache;

    std::string nullvalue;
    std::string root_string;
};
