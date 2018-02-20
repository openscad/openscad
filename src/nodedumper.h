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
        NodeDumper(NodeCache &cache, const std::string& indent = "", bool idPrefix = false) :
                cache(cache), indent(indent), idprefix(idPrefix), currindent(0), root(nullptr) { }
        ~NodeDumper() {}

        Response visit(State &state, const AbstractNode &node) override;
        Response visit(State &state, const RootNode &node) override;

private:
        void handleVisitedChildren(const State &state, const AbstractNode &node);

        NodeCache &cache;
        std::string indent;
        bool idprefix;

        int currindent;
        const AbstractNode *root;
        std::stringstream dump;

};
