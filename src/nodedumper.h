#pragma once

#include <string>
#include <map>
#include <list>
#include "NodeVisitor.h"
#include "node.h"
#include "nodecache.h"

class NodeDumper : public NodeVisitor
{
public:
        /*! If idPrefix is true, we will output "n<id>:" in front of each node,
          which is useful for debugging. */
        NodeDumper(NodeCache &cache, bool idPrefix = false) :
                cache(cache), idprefix(idPrefix), root(nullptr) { }
        ~NodeDumper() {}

        Response visit(State &state, const AbstractNode &node) override;
        Response visit(State &state, const RootNode &node) override;

private:
        void handleVisitedChildren(const State &state, const AbstractNode &node);
        bool isCached(const AbstractNode &node) const;
        void handleIndent(const State &state);
        std::string dumpChildBlock(const AbstractNode &node);
        std::string dumpChildren(const AbstractNode &node);

        NodeCache &cache;
        bool idprefix;

        std::string currindent;
        const AbstractNode *root;
        typedef std::list<const AbstractNode *> ChildList;
        std::map<int, ChildList> visitedchildren;
};
