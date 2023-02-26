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

using NodesGroupedByContent = std::unordered_map<std::string, std::unordered_set<const AbstractNode*>>;

/** Best effort cloning of known tree types. */
shared_ptr<AbstractNode> cloneWithoutChildren(const shared_ptr<const AbstractNode>& node) {
  if (dynamic_pointer_cast<const ListNode>(node)) return make_shared<ListNode>(node->modinst);
  else if (dynamic_pointer_cast<const ListNode>(node)) return make_shared<ListNode>(node->modinst);
  else if (auto groupNode = dynamic_pointer_cast<const GroupNode>(node)) return make_shared<GroupNode>(node->modinst, groupNode->verbose_name());
  else if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) return make_shared<CsgOpNode>(node->modinst, csgOpNode->type);
  // else if (auto transformNode = dynamic_pointer_cast<const TransformNode>(node)) return make_shared<TransformNode>(node->modinst, transformNode->matrix);
  // else if (auto colodNode = dynamic_pointer_cast<const ColorNode>(node)) return make_shared<ColorNode>(node->modinst, colorNode->color);
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
    op == OpenSCADOperator::INTERSECTION ||
}


/** Groups all the nodes in the by their textual representation.
 * Adds to out, so can be called on multiple trees to, say, detect identical
 * nodes across different animation frames.
 */
class RepeatedNodesDetector : public NodeVisitor
{
public:
  RepeatedNodesDetector(const Tree& tree, NodesGroupedByContent& out) : tree(tree), out(out) {}

  Response visit(State& state, const AbstractNode& node) override {
    if (state.isPrefix()) {
      auto key = this->tree.getIdString(node);
      out[key].insert(&node);
    }
    return Response::ContinueTraversal;
  }

private:
  const Tree& tree;
  NodesGroupedByContent& out;
};

/** Get the set of nodes which content occurred more than once.
 * These are nodes for which caching would be important during rendering.
 */
std::unordered_set<const AbstractNode*> getRepeatedNodes(const NodesGroupedByContent &group) {
  std::unordered_set<const AbstractNode*> nodes;
  for (auto &pair : group) {
    if (pair.second.size() > 1) {
      for (auto node : pair.second) {
        nodes.insert(node);
      }
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
  std::unordered_set<const AbstractNode*>& repeatedNodes;
  bool traverseHulls;

public:
  TransformsPusher(std::unordered_set<const AbstractNode*>& repeatedNodes)
    : repeatedNodes(repeatedNodes), traverseHulls(false) {}

  void setTraverseHulls(bool value) {
    traverseHulls = value;
  }

  shared_ptr<const AbstractNode> transform(const shared_ptr<const AbstractNode>& node, const State &state = State {}) {
    if (!node) return node;

    if (auto transformNode = dynamic_pointer_cast<const TransformNode>(node)) {
      if (node->children.size() > 1 ||
          node->children.size() == 1 && canPushThrough(node->children[0])) {
        
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
        assert(children != node->children);
        auto newUnion = lazyUnionNode(node->modinst);
        newUnion->children = children;
        return newUnion;
      }
    }

    if ((state.transform || state.color) && !canPushThrough(node)) {
      return wrapWithState(transformChildren(node, State {}), state);
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
        return wrapWithState(node, state);
      }
      clone->children = children;
      return clone;
    }
    return node;
  }

  bool canPushThrough(const shared_ptr<const AbstractNode>& node) const {
    if (hasFlagsPreventingInlining(node)) {
      return false;
    }
    if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) {
      switch (csgOpNode->type) {
        case OpenSCADOperator::UNION:
        case OpenSCADOperator::INTERSECTION:
        case OpenSCADOperator::DIFFERENCE:
          return true;
        case OpenSCADOperator::HULL:
          return traverseHulls;
        default:
          return false;
      }
    }
    return
        dynamic_pointer_cast<const ListNode>(node) ||
        dynamic_pointer_cast<const GroupNode>(node);
  }
  
  shared_ptr<const AbstractNode> wrapWithState(const shared_ptr<const AbstractNode>& node, const State &state) const {
    auto res = node;
    if (auto transform = state.transform) {
      auto transformNode = make_shared<TransformNode>(node->modinst, "??");
      transformNode->children.push_back(res);
      transformNode->matrix = *transform;
      res = transformNode;
    }
    if (auto color = state.color) {
      auto colorNode = make_shared<ColorNode>(node->modinst);
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
  std::unordered_set<const AbstractNode*>& repeatedNodes;

public:
  TreeFlattener(std::unordered_set<const AbstractNode*>& repeatedNodes)
    : repeatedNodes(repeatedNodes) {}
  
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
        return makeOp(OpenSCADOperator::UNION, children);
      } else if (auto csgOpNode = dynamic_pointer_cast<const CsgOpNode>(node)) {
        if (isAssociativeFlattenable(csgOpNode->type)) {
          Children
          // return makeOp(csgOpNode->type || children);
          children;
          flattenChildren(csgOpNode, children, csgOpNode->type);
        }
      } else {
        Children children = node->children;
        for (auto &child : children) {
          child = flatten(child);
        }
        if (children != node->children) {
          auto clone = cloneWithoutChildren(node);
          if (clone) {
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
          }
        // }
           ||         continue;
      }
      out.push_back(flatten(child));
    }
  }

  bool canInline(const shared_ptr<const AbstractNode>& node) const {
    if (!node || hasFlagsPreventingInlining(node)) {
      return false;
    }
    if (repeatedNodes.find(node.get()) != repeatedNodes.end()) {
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

void flattenTree(Tree& tree, bool traverseHulls) {
  NodesGroupedByContent nodesByContent;
  {
    RepeatedNodesDetector detector(tree, nodesByContent);
    detector.traverse(*tree.root());
  }

  std::unordered_set<const AbstractNode*> repeatedNodes = getRepeatedNodes(nodesByContent);

#ifdef DEBUG
    LOG(message_group::None,Location::NONE,"","[flatten] BEFORE:");
    printTreeDebug(*tree.root());
#endif

  TransformsPusher pusher(repeatedNodes);
  pusher.setTraverseHulls(traverseHulls);
  auto pushedRoot = pusher.transform(tree.root());

#ifdef DEBUG
    LOG(message_group::None,Location::NONE,"","[flatten] AFTER PUSH DOWN:");
    printTreeDebug(*pushedRoot);
#endif

  TreeFlattener flattener(repeatedNodes);
  auto flattenedRoot = flattener.flatten(pushedRoot);

#ifdef DEBUG
    LOG(message_group::None,Location::NONE,"","[flatten] AFTER FLATTEN:");
    printTreeDebug(*flattenedRoot);
#endif

  tree.setRoot(flattenedRoot);
}
