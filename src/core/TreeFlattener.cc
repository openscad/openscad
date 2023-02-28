#include "TreeFlattener.h"

#include "ColorNode.h"
#include "CsgOpNode.h"
#include "node.h"
#include "NodeVisitor.h"
#include "TransformNode.h"
#include "Tree.h"

#include <unordered_set>
#include <unordered_map>
#include <stack>

using NodeIds = std::unordered_set<std::string>;
// using NodesGroupedByContent = std::unordered_map<std::string, std::shared_ptr<const AbstractNode>>;
using NodeIdOccurrences = std::unordered_map<std::string, size_t>;

/** Best effort cloning of known tree types. */
shared_ptr<AbstractNode> cloneWithoutChildren(const shared_ptr<const AbstractNode>& node) {
  if (dynamic_pointer_cast<const ListNode>(node)) return make_shared<ListNode>(node->modinst);
  else if (dynamic_pointer_cast<const ListNode>(node)) return make_shared<ListNode>(node->modinst);
  else if (auto groupNode = dynamic_pointer_cast<const GroupNode>(node)) return make_shared<GroupNode>(node->modinst, groupNode->verbose_name());
  else if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) return make_shared<CsgOpNode>(node->modinst, csgOpNode->type);
  else if (auto transformNode = dynamic_pointer_cast<const TransformNode>(node)) {
    auto ret = make_shared<TransformNode>(node->modinst, transformNode->verbose_name());
    ret->matrix = transformNode->matrix;
    return ret;
  }
  else if (auto colorNode = dynamic_pointer_cast<const ColorNode>(node)) {
    auto ret = make_shared<ColorNode>(node->modinst);
    ret->color = colorNode->color;
    return ret;
  }
  else return nullptr;
}

/** Detect flags on nodes that should not be inlined.
 * Not testing for isRoot, as we'll be called on the actual tree root
 * to only render what's below it.
 */
bool hasFlagsPreventingInlining(const shared_ptr<const AbstractNode>& node) {
  return node && node->modinst &&
      (node->modinst->isBackground() || node->modinst->isHighlight());
}

bool isAssociativeFlattenable(OpenSCADOperator op) {
  return
    // op == OpenSCADOperator::HULL ||
    op == OpenSCADOperator::UNION ||
    op == OpenSCADOperator::INTERSECTION;
}

bool isFakeRepetition(const AbstractNode& node) {
  // The id string of `group() X();` is the same as `X();`.
  return dynamic_cast<const GroupNode*>(&node) && node.children.size() == 1;
}

/** Groups all the nodes in the by their textual representation.
 * Adds to out, so can be called on multiple trees to, say, detect identical
 * nodes across different animation frames.
 */
class RepeatedNodesDetector : public NodeVisitor
{
public:
  RepeatedNodesDetector(const Tree& tree, NodeIdOccurrences& occurrences) : tree(tree), occurrences(occurrences) {}

  Response visit(State& state, const AbstractNode& node) override {
    if (state.isPrefix()) {
      if (!isFakeRepetition(node)) {
        auto key = this->tree.getIdString(node);
        if (++occurrences[key] > 1) {
          return Response::PruneTraversal;      
        }
      }
    }
    return Response::ContinueTraversal;
  }

private:

  const Tree& tree;
  NodeIdOccurrences& occurrences;
};

/** Get the set of nodes which content occurred more than once.
 * These are nodes for which caching would be important during rendering.
 */
NodeIds getRepeatedNodeIds(const NodeIdOccurrences &occurrences) {
  NodeIds nodes;
  for (auto &pair : occurrences) {
    if (pair.second > 1) {
      nodes.insert(pair.first);
    }
  }
  return std::move(nodes);
}

/** Transforms trees by pushing transform nodes down through many operations.
 *
 * Avoids pushing through repeated nodes, which would defeat caching, or through nodes w/ special marks.
 */
class TransformsPusher
{
  using Children = std::vector<shared_ptr<const AbstractNode>>;
  struct State {
    std::optional<Transform3d> transform;
    std::optional<Color4f> color;
  };
  NodeIds& repeatedNodeIds;
  const Tree &tree;

public:
  TransformsPusher(const Tree &tree, NodeIds& repeatedNodeIds)
    : tree(tree), repeatedNodeIds(repeatedNodeIds) {}

  shared_ptr<const AbstractNode> transform(const shared_ptr<const AbstractNode>& node, const State &state = State {}) {
    if (!node) return node;

    if (canInline(node)) {
      if (auto transformNode = dynamic_pointer_cast<const TransformNode>(node)) {
        State newState = state;
        if (auto transform = state.transform) {
          newState.transform = *transform * transformNode->matrix;
        } else {
          newState.transform = transformNode->matrix;
        }

        Children children = node->children;
        for (auto &child : children) {
          child = transform(child, newState);
        }
        if (children.size() == 1) {
          return children[0];
        } else {
          auto newUnion = lazyUnionNode(node->modinst);
          newUnion->children = children;
          return newUnion;
        }
      }
    }

    if ((state.transform || state.color) && !canPushThrough(node)) {
      return wrapWithState(transformChildren(node, State {}), state, node->modinst);
    } else {
      return transformChildren(node, state);
    }
  }

private:

  shared_ptr<const AbstractNode> transformChildren(const shared_ptr<const AbstractNode>& node, const State &state) {
    Children children = node->children;
    for (auto &child : children) {
      child = transform(child, state);
    }
    if (children != node->children) {
      auto clone = cloneWithoutChildren(node);
      assert(clone);
      if (!clone) {
        return wrapWithState(node, state, node->modinst);
      }
      clone->children = children;
      return clone;
    }
    return node;
  }

  bool canInline(const shared_ptr<const AbstractNode>& node) const {
    if (!node || hasFlagsPreventingInlining(node)) {
      return false;
    }
    auto id = tree.getIdString(*node);
    if (!isFakeRepetition(*node) && repeatedNodeIds.find(id) != repeatedNodeIds.end()) {
      return false;
    }
    return true;
  }

  bool canPushThrough(const shared_ptr<const AbstractNode>& node) const {
    if (!canInline(node)) {
      return false;
    }
    if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) {
      switch (csgOpNode->type) {
        case OpenSCADOperator::UNION:
        case OpenSCADOperator::INTERSECTION:
        case OpenSCADOperator::DIFFERENCE:
          return true;
        case OpenSCADOperator::HULL:
          // Not pushing transforms through hulls as this would apply
          // them to potentially many more points. It could however
          // improve overall accurracy when using Manifold geometry.
        default:
          return false;
      }
    }
    return
        dynamic_pointer_cast<const ListNode>(node) ||
        dynamic_pointer_cast<const GroupNode>(node);
  }
  
  shared_ptr<const AbstractNode> wrapWithState(const shared_ptr<const AbstractNode>& node, const State &state, const ModuleInstantiation *modinst) const {
    auto res = node;
    if (auto transform = state.transform) {
      auto transformNode = make_shared<TransformNode>(modinst, "??");
      transformNode->children.push_back(res);
      transformNode->matrix = *transform;
      res = transformNode;
    }
    if (auto color = state.color) {
      auto colorNode = make_shared<ColorNode>(modinst);
      colorNode->children.push_back(res);
      colorNode->color = *color;
      res = colorNode;
    }
    return res;
  }
};

/**
 * Flattens nodes w/ associative operations, treating ListNode & GroupNode as unions.
 */
class TreeFlattener
{
  const Tree &tree;
  NodeIds& repeatedNodeIds;

public:
  TreeFlattener(const Tree &tree, NodeIds& repeatedNodeIds)
    : tree(tree), repeatedNodeIds(repeatedNodeIds) {}
  
  shared_ptr<const AbstractNode> flatten(const shared_ptr<const AbstractNode>& node) {
    if (!node) return node;

    using Children = std::vector<shared_ptr<const AbstractNode>>;

    if (canInline(node)) {
      auto makeOp = [&](OpenSCADOperator op, const Children& children) -> shared_ptr<const AbstractNode> {
        if (children.size() == 1) {
          return children[0];
        }
        if (children == node->children) {
          return node;
        } 

        shared_ptr<AbstractNode> ret;
        if (op == OpenSCADOperator::UNION) {
          ret = lazyUnionNode(node->modinst);
        } else {
          ret = make_shared<CsgOpNode>(node->modinst, op);
        }
        ret->children = children;
        return ret;
      };

      if (dynamic_pointer_cast<const ListNode>(node) || 
          dynamic_pointer_cast<const GroupNode>(node)) {
        Children children;
        flattenChildren(node, children, OpenSCADOperator::UNION);
        if (children != node->children) {
          return makeOp(OpenSCADOperator::UNION, children);
        }
      } else if (dynamic_pointer_cast<const TransformNode>(node) ||
                 dynamic_pointer_cast<const ColorNode>(node)) {
        Children children;
        flattenChildren(node, children, OpenSCADOperator::UNION);
        if (children != node->children) {
          if (auto clone = cloneWithoutChildren(node)) {
            clone->children = children;
            return clone;
          }
        }
      } else if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) {
        if (isAssociativeFlattenable(csgOpNode->type)) {
          Children children;
          flattenChildren(csgOpNode, children, csgOpNode->type);
          if (children != node->children) {
            return makeOp(csgOpNode->type, children);
          }
        }
      } else {
        Children children = node->children;
        for (auto &child : children) {
          child = flatten(child);
        }
        if (children != node->children) {
          if (auto clone = cloneWithoutChildren(node)) {
            clone->children = children;
            return clone;
          }
        }
      }
    }
    return node;
  }

private:

  void flattenChildren(const shared_ptr<const AbstractNode>& node, std::vector<shared_ptr<const AbstractNode>> &out, const std::optional<OpenSCADOperator> allowedOp) {
    if (!node) {
      return;
    }
    for (auto child : node->children) {
      if (canInline(child)) {
        if (dynamic_pointer_cast<const ListNode>(child) || dynamic_pointer_cast<const GroupNode>(child)) {
          flattenChildren(child, out, allowedOp);
          continue;
        }
        if (auto csgNode = dynamic_pointer_cast<const CsgOpNode>(child)) {
          if (allowedOp && csgNode->type == *allowedOp && isAssociativeFlattenable(*allowedOp)) {
            flattenChildren(csgNode, out, allowedOp);
            continue;
          }
        }
      }
      out.push_back(flatten(child));
    }
  }

  bool canInline(const shared_ptr<const AbstractNode>& node) const {
    if (!node || hasFlagsPreventingInlining(node)) {
      return false;
    }
    auto id = tree.getIdString(*node);
    if (!isFakeRepetition(*node) && repeatedNodeIds.find(id) != repeatedNodeIds.end()) {
      return false;
    }
    return true;
  }
};

void printTreeDebug(const AbstractNode& node, const std::string& indent = "") {
  auto hasChildren = node.getChildren().size() > 0;
  LOG(message_group::None,Location::NONE,"", "%1$s",
      (indent + node.toString() + (hasChildren ? " {" : ";")).c_str());
  for (const auto child : node.getChildren()) {
    if (child) printTreeDebug(*child, indent + "  ");
  }
  if (hasChildren) LOG(message_group::None,Location::NONE,"", "%1$s", (indent + "}").c_str());
}

void flattenTree(Tree& tree) {
  if (!tree.root()) {
    return;
  }
  NodeIdOccurrences occurrences;
  RepeatedNodesDetector detector(tree, occurrences);
  detector.traverse(*tree.root());

  NodeIds repeatedNodeIds = getRepeatedNodeIds(occurrences);

// #ifdef DEBUG
//   for (const auto &id : repeatedNodeIds) {
//     LOG(message_group::None,Location::NONE,"","[flatten] REPEATED: %1$s", id.c_str());
//   }
//     LOG(message_group::None,Location::NONE,"","[flatten] BEFORE:");
//     printTreeDebug(*tree.root());
// #endif

  TransformsPusher pusher(tree, repeatedNodeIds);
  tree.setRoot(pusher.transform(tree.root()));

// #ifdef DEBUG
//     LOG(message_group::None,Location::NONE,"","[flatten] AFTER PUSH DOWN:");
//     printTreeDebug(*tree.root());
// #endif

  TreeFlattener flattener(tree, repeatedNodeIds);
  tree.setRoot(flattener.flatten(tree.root()));

// #ifdef DEBUG
//     LOG(message_group::None,Location::NONE,"","[flatten] AFTER FLATTEN:");
//     printTreeDebug(*tree.root());
// #endif
}
