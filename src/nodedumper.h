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
        NodeDumper(NodeCache &cache, const std::string& indent, bool idString, bool idPrefix) :
                cache(cache), indent(indent), idString(idString), idprefix(idPrefix), currindent(0), root(nullptr) { }
        ~NodeDumper() {}

        Response visit(State &state, const AbstractNode &node) override;
        Response visit(State &state, const RootNode &node) override;

private:
        bool isCached(const AbstractNode &node) const;

        NodeCache &cache;
        std::string indent;
        bool idString;
        bool idprefix;

        int currindent;
        const AbstractNode *root;
        std::stringstream dump;

};
