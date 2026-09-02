#include "core/RemoveNodeRewriter.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <iterator>

#include "core/CsgOpNode.h"
#include "core/ModuleInstantiation.h"
#include "core/RemoveNode.h"
#include "core/enums.h"

namespace {

using NodeList = std::vector<std::shared_ptr<AbstractNode>>;
using Range = std::pair<NodeList::const_iterator, NodeList::const_iterator>;
using SegmentList = std::vector<Range>;

auto isRemoveNode = [](const std::shared_ptr<AbstractNode>& node) -> bool {
  return nullptr != std::dynamic_pointer_cast<RemoveNode>(node);
};

void applyRemoveModifiers(AbstractNode& remNode)
{
  auto *mi = const_cast<ModuleInstantiation *>(remNode.modinst);
  if (!(mi->tag_root || mi->tag_highlight || mi->tag_background)) return;

  for (auto& child : remNode.children) {
    auto *childMi = const_cast<ModuleInstantiation *>(child->modinst);
    childMi->tag_root |= mi->tag_root;
    childMi->tag_highlight |= mi->tag_highlight;
    childMi->tag_background |= mi->tag_background;
  }
  mi->tag_root = mi->tag_highlight = mi->tag_background = false;
}

std::shared_ptr<AbstractNode> createUnion(const ModuleInstantiation *mi, Range nodeList)
{
  const auto cnt = std::distance(nodeList.first, nodeList.second);
  if (cnt == 0) return std::make_shared<GroupNode>(mi);
  if (cnt == 1) return *nodeList.first;
  auto node = std::make_shared<CsgOpNode>(mi, OpenSCADOperator::UNION);
  std::vector<std::shared_ptr<AbstractNode>> children;
  children.insert(children.begin(), nodeList.first, nodeList.second);
  node->children = std::move(children);
  return node;
}

SegmentList groupNodes(const NodeList& nodeList)
{
  if (nodeList.empty()) return {};

  auto start_it = nodeList.begin();

  SegmentList segments;
  bool current_state = isRemoveNode(*start_it);
  if (current_state) segments.emplace_back(start_it, start_it);

  for (auto next_it = nodeList.begin() + 1; next_it != nodeList.end(); ++next_it) {
    bool next_state = isRemoveNode(*next_it);
    if (next_state != current_state) {
      segments.emplace_back(start_it, next_it);
      start_it = next_it;
      current_state = next_state;
    }
  }

  segments.emplace_back(start_it, nodeList.end());
  if (segments.size() % 2) {
    segments.emplace_back(nodeList.end(), nodeList.end());
  }

  return segments;
}

NodeList rewriteSpans(const ModuleInstantiation *mi, const SegmentList& segmentList)
{
  NodeList out;

  for (size_t i = 0; i < segmentList.size(); i += 2) {
    const auto& [posStart, posEnd] = segmentList[i];
    const auto& [negStart, negEnd] = segmentList[i + 1];

    std::vector<std::shared_ptr<AbstractNode>> negChildren;
    for (auto it = negStart; it != negEnd; it++) {
      const auto& negNode = *it;
      applyRemoveModifiers(*negNode);
      negChildren.insert(negChildren.end(), negNode->children.begin(), negNode->children.end());
    }

    if (std::distance(negStart, negEnd) == 0) {
      out.insert(out.end(), posStart, posEnd);
    } else {
      auto diff = std::make_shared<CsgOpNode>(mi, OpenSCADOperator::DIFFERENCE);
      diff->children.push_back(createUnion(mi, segmentList[i]));
      diff->children.insert(diff->children.end(), negChildren.begin(), negChildren.end());
      out.push_back(std::move(diff));
    }
  }

  return out;
}

bool rewriteNode(AbstractNode& node)
{
  bool hasRemove = false;
  for (auto& child : node.children) {
    if (rewriteNode(*child)) hasRemove = true;
  }

  if (!hasRemove && std::none_of(node.children.begin(), node.children.end(), isRemoveNode)) {
    return false;
  }

  node.children = rewriteSpans(node.modinst, groupNodes(node.children));
  return true;
}

}  // namespace

void rewriteRemoveNodes(AbstractNode& node)
{
  rewriteNode(node);
}
