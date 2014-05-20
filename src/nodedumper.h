#pragma once

#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "nodecache.h"

class NodeDumper : public Visitor
{
public:
        /*! If idPrefix is true, we will output "n<id>:" in front of each node,
          which is useful for debugging. */
        NodeDumper(NodeCache &cache, bool idPrefix = false) :
                cache(cache), idprefix(idPrefix), root(NULL) { }
        virtual ~NodeDumper() {}

        virtual Response visit(State &state, const AbstractNode &node);

private:
        void handleVisitedChildren(const State &state, const AbstractNode &node);
        bool isCached(const AbstractNode &node) const;
        void handleIndent(const State &state);
        std::string dumpChildren(const AbstractNode &node);

        NodeCache &cache;
        bool idprefix;

        std::string currindent;
        const AbstractNode *root;
        typedef std::list<const AbstractNode *> ChildList;
        std::map<int, ChildList> visitedchildren;
};
