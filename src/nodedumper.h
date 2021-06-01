#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include "NodeVisitor.h"
#include "node.h"
#include "nodecache.h"

// GroupNodeChecker does a quick first pass to count children of group nodes
// If a GroupNode has 0 children, don't include in node id strings
// If a GroupNode has 1 child, we replace it with its child
// This makes id strings much more compact for deeply nested trees, recursive scad scripts,
// and increases likelihood of node cache hits.
class GroupNodeChecker : public NodeVisitor 
{
public:
    GroupNodeChecker(){}

    Response visit(State &state, const AbstractNode &node) override;
    Response visit(State &state, const GroupNode &node) override;
    void incChildCount(int groupNodeIndex);
    int getChildCount(int groupNodeIndex) const;
    void reset() { groupChildCounts.clear(); }

private:
    // stores <node_idx,nonEmptyChildCount> for each group node
    std::unordered_map<int, int> groupChildCounts;
};

class NodeDumper : public NodeVisitor
{
public:
    NodeDumper(NodeCache &cache, const AbstractNode *root_node, const std::string& indent, bool idString) :
            cache(cache), indent(indent), idString(idString), currindent(0), root(root_node) { 
        if (idString) { 
            groupChecker.traverse(*root);
        }
    }
    ~NodeDumper() {}

    Response visit(State &state, const AbstractNode &node) override;
    Response visit(State &state, const GroupNode &node) override;
    Response visit(State &state, const ListNode &node) override;
    Response visit(State &state, const RootNode &node) override;

private:
    void initCache();
    void finalizeCache();
    bool isCached(const AbstractNode &node) const;

    NodeCache &cache;
    // Output Formatting options
    std::string indent;
    bool idString;

    int currindent;
    const AbstractNode *root;
    GroupNodeChecker groupChecker;
    std::ostringstream dumpstream;

};


