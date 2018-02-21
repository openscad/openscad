#pragma once

#include <vector>
#include <string>
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
    virtual ~NodeCache() { }

    bool contains(const AbstractNode &node) const {
        auto i = node.index();
        return i > 0 && 
            endcache.size() >= i &&
            endcache[i-1] >= 0; /*  && 
            startcache.size() >= i && 
            frontcache[i-1] >= 0;
            */
    }

    const std::string operator[](const AbstractNode &node) const {
        auto i = node.index();
        // node.index() is 1-based
        assert(i >= 1 && "unexpected index < 1");
        long start = startcache[i-1];
        long end = endcache[i-1];
        return root_string.substr(start, end-start);
    }

    void insert_start(const class AbstractNode &node, long strindex) {
        auto i = node.index();
        // node.index() is 1-based
        assert(i >= 1 && "unexpected index < 1");
        if (startcache.size() < i) {
            startcache.resize(i, -1L);
            endcache.resize(i, -1L);
        }
        assert(startcache[i-1] == -1L && "start index inserted twice");
        startcache[i-1] = strindex;
    }

    void insert_end(const class AbstractNode &node, long strindex) {
        auto i = node.index();
        // node.index() is 1-based
        assert(i >= 1 && "unexpected index < 1");
        assert(endcache.size() >= i && "inserted end index before start?");
        assert(endcache[i-1] == -1L && "end index inserted twice");
        endcache[i-1] = strindex;
        PRINTDB("NodeCache Insert nodecache[%i] = [%d:%d]", i % startcache[i-1] % endcache[i-1] );
    }

    void set_root_string(const std::string &root_str) {
        root_string = root_str;
    }

    void clear() {
        startcache.clear();
        endcache.clear();
        root_string = "";
    }

private:
    std::vector<long> startcache;
    std::vector<long> endcache;

    std::string nullvalue;
    std::string root_string;
};
