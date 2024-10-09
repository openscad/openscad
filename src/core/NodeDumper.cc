#include "core/NodeDumper.h"
#include "core/State.h"
#include "core/ModuleInstantiation.h"
#include <algorithm>
#include <iterator>
#include <ostream>
#include <string>
#include <sstream>
#include <boost/regex.hpp>


void GroupNodeChecker::incChildCount(int groupNodeIndex) {
  auto search = this->groupChildCounts.find(groupNodeIndex);
  // if no entry then given node wasn't a group node
  if (search != this->groupChildCounts.end()) {
    ++(search->second);
  }
}

int GroupNodeChecker::getChildCount(int groupNodeIndex) const {
  auto search = this->groupChildCounts.find(groupNodeIndex);
  if (search != this->groupChildCounts.end()) {
    return search->second;
  } else {
    return -1;
  }
}

Response GroupNodeChecker::visit(State& state, const GroupNode& node)
{
  if (state.isPrefix()) {
    // create entry for group node, which children may increment
    this->groupChildCounts.emplace(node.index(), 0);
  } else if (state.isPostfix()) {
    if ((this->getChildCount(node.index()) > 0) && state.parent()) {
      this->incChildCount(state.parent()->index());
    }
  }
  return Response::ContinueTraversal;
}

Response GroupNodeChecker::visit(State& state, const AbstractNode&)
{
  if (state.isPostfix() && state.parent()) {
    this->incChildCount(state.parent()->index());
  }
  return Response::ContinueTraversal;
}


/*!
   \class NodeDumper

   A visitor responsible for creating a text dump of a node tree.  Also
   contains a cache for fast retrieval of the text representation of
   any node or subtree.
 */

void NodeDumper::initCache()
{
  this->dumpstream.str("");
  this->dumpstream.clear();
  this->cache.clear();
}

void NodeDumper::finalizeCache()
{
  this->cache.setRootString(this->dumpstream.str());
}

bool NodeDumper::isCached(const AbstractNode& node) const
{
  return this->cache.contains(node);
}

Response NodeDumper::visit(State& state, const GroupNode& node)
{
  if (!this->idString) {
    return NodeDumper::visit(state, (const AbstractNode&)node);
  }
  if (state.isPrefix()) {
    // For handling root modifier '!'
    // Check if we are processing the root of the current Tree and init cache
    if (this->root.get() == &node) {
      this->initCache();
    }

    // ListNodes can pass down modifiers to children via state, so check both modinst and state
    if (node.modinst->isBackground() || state.isBackground()) this->dumpstream << "%";
    if (node.modinst->isHighlight() || state.isHighlight()) this->dumpstream << "#";

// If IDPREFIX is set, we will output "/*id*/" in front of each node
// which is useful for debugging.
#ifdef IDPREFIX
    if (this->idString) this->dumpstream << "\n";
    this->dumpstream << "/*" << node.index() << "*/";
#endif

    // insert start index
    this->cache.insertStart(node.index(), this->dumpstream.tellp());

    if (this->groupChecker.getChildCount(node.index()) > 1) {
      this->dumpstream << node << "{";
    }
    this->currindent++;
  } else if (state.isPostfix()) {
    this->currindent--;
    if (this->groupChecker.getChildCount(node.index()) > 1) {
      this->dumpstream << "}";
    }
    // insert end index
    this->cache.insertEnd(node.index(), this->dumpstream.tellp());

    // For handling root modifier '!'
    // Check if we are processing the root of the current Tree and finalize cache
    if (this->root.get() == &node) {
      this->finalizeCache();
    }
  }

  return Response::ContinueTraversal;
}


/*!
   Called for each node in the tree.
 */
Response NodeDumper::visit(State& state, const AbstractNode& node)
{
  if (state.isPrefix()) {

    // For handling root modifier '!'
    // Check if we are processing the root of the current Tree and init cache
    if (this->root.get() == &node) {
      this->initCache();
    }

    // ListNodes can pass down modifiers to children via state, so check both modinst and state
    if (node.modinst->isBackground() || state.isBackground()) this->dumpstream << "%";
    if (node.modinst->isHighlight() || state.isHighlight()) this->dumpstream << "#";

// If IDPREFIX is set, we will output "/*id*/" in front of each node
// which is useful for debugging.
#ifdef IDPREFIX
    if (this->idString) this->dumpstream << "\n";
    this->dumpstream << "/*" << node.index() << "*/";
#endif

    // insert start index
    this->cache.insertStart(node.index(), this->dumpstream.tellp());

    if (this->idString) {

      static const boost::regex re(R"([^\s\"]+|\"(?:[^\"\\]|\\.)*\")");
      const auto name = STR(node);
      boost::sregex_token_iterator it(name.begin(), name.end(), re, 0);
      std::copy(it, boost::sregex_token_iterator(), std::ostream_iterator<std::string>(this->dumpstream));

      if (node.getChildren().size() > 0) {
        this->dumpstream << "{";
      }

    } else {

      for (int i = 0; i < this->currindent; ++i) {
        this->dumpstream << this->indent;
      }
      this->dumpstream << node;
      if (node.getChildren().size() > 0) {
        this->dumpstream << " {\n";
      }
    }

    this->currindent++;

  } else if (state.isPostfix()) {

    this->currindent--;

    if (this->idString) {
      if (node.getChildren().size() > 0) {
        this->dumpstream << "}";
      } else {
        this->dumpstream << ";";
      }
    } else {
      if (node.getChildren().size() > 0) {
        for (int i = 0; i < this->currindent; ++i) {
          this->dumpstream << this->indent;
        }
        this->dumpstream << "}\n";
      } else {
        this->dumpstream << ";\n";
      }
    }

    // insert end index
    this->cache.insertEnd(node.index(), this->dumpstream.tellp());

    // For handling root modifier '!'
    // Check if we are processing the root of the current Tree and finalize cache
    if (this->root.get() == &node) {
      this->finalizeCache();
    }
  }

  return Response::ContinueTraversal;
}

/*!
   Handle list nodes specially: Only list children
 */
Response NodeDumper::visit(State& state, const ListNode& node)
{
  if (state.isPrefix()) {
    // For handling root modifier '!'
    if (this->root.get() == &node) {
      this->initCache();
    }
    // pass modifiers down to children via state
    if (node.modinst->isHighlight()) state.setHighlight(true);
    if (node.modinst->isBackground()) state.setBackground(true);
    this->cache.insertStart(node.index(), this->dumpstream.tellp());
  } else if (state.isPostfix()) {
    this->cache.insertEnd(node.index(), this->dumpstream.tellp());
    // For handling root modifier '!'
    if (this->root.get() == &node) {
      this->finalizeCache();
    }
  }

  return Response::ContinueTraversal;
}

/*!
   Handle root nodes specially: Only list children
 */
Response NodeDumper::visit(State& state, const RootNode& node)
{
  if (isCached(node)) return Response::PruneTraversal;

  if (state.isPrefix()) {
    this->initCache();
    this->cache.insertStart(node.index(), this->dumpstream.tellp());
  } else if (state.isPostfix()) {
    this->cache.insertEnd(node.index(), this->dumpstream.tellp());
    this->finalizeCache();
  }

  return Response::ContinueTraversal;
}
