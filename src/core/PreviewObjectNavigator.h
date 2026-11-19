#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include "core/CgalAdvNode.h"
#include "core/ColorNode.h"
#include "core/RenderNode.h"
#include "core/TransformNode.h"
#include "core/node.h"

// Keeps navigation tied to one evaluated preview tree. No AST or source text is changed.
class PreviewObjectNavigator
{
public:
  void reset(const std::shared_ptr<const AbstractNode>& root)
  {
    clear();
    if (!root) return;
    std::vector<std::shared_ptr<const AbstractNode>> pending{root};
    std::vector<std::shared_ptr<const AbstractNode>> ordered;
    while (!pending.empty()) {
      auto node = pending.back();
      pending.pop_back();
      ordered.push_back(node);
      // These operations produce one CSG leaf. Their input geometry has no
      // independently selectable object in the compiled preview.
      if (isGeometryLeaf(*node)) continue;
      for (auto child = node->children.rbegin(); child != node->children.rend(); ++child) {
        pending.push_back(*child);
      }
    }

    // Skip echo/assert-only branches and empty structural groups.
    std::unordered_set<int> geometric;
    for (auto node = ordered.rbegin(); node != ordered.rend(); ++node) {
      if (isGeometryLeaf(**node) ||
          std::any_of((*node)->children.begin(), (*node)->children.end(),
                      [&geometric](const auto& child) { return geometric.count(child->index()) > 0; })) {
        geometric.insert((*node)->index());
      }
    }
    for (const auto& node : ordered) {
      // Transform and color nodes only modify their descendants; they are not
      // independently selectable objects in the preview.
      if (geometric.count(node->index()) && !isModifier(*node) && node->modinst &&
          !node->modinst->location().isNone()) {
        objects.push_back(node);
      }
    }
  }

  void clear()
  {
    objects.clear();
    rewind();
  }

  void rewind() { position = -1; }
  [[nodiscard]] size_t size() const { return objects.size(); }
  [[nodiscard]] int currentPosition() const { return position; }
  [[nodiscard]] std::vector<int> otherObjectIndices() const
  {
    std::vector<int> result;
    if (position < 0 || position >= static_cast<int>(objects.size())) return result;
    const auto& selected = objects[position];
    for (size_t i = 0; i < objects.size(); ++i) {
      if (static_cast<int>(i) == position) continue;
      const auto& candidate = objects[i];
      // Avoid ghosting an enclosing CSG operation (which contains the selected
      // object) or the selected operation's own children.
      if (containsIndex(*candidate, selected->index()) ||
          containsIndex(*selected, candidate->index())) {
        continue;
      }
      result.push_back(candidate->index());
    }
    return result;
  }

  std::shared_ptr<const AbstractNode> step(int direction)
  {
    if (objects.empty()) return nullptr;
    if (position < 0) position = direction < 0 ? static_cast<int>(objects.size()) - 1 : 0;
    else if (direction > 0 && position + 1 < static_cast<int>(objects.size())) ++position;
    else if (direction < 0 && position > 0) --position;
    return objects[position];
  }

private:
  static bool isModifier(const AbstractNode& node)
  {
    return dynamic_cast<const TransformNode *>(&node) || dynamic_cast<const ColorNode *>(&node);
  }

  static bool containsIndex(const AbstractNode& root, int index)
  {
    std::vector<const AbstractNode *> pending{&root};
    while (!pending.empty()) {
      const auto node = pending.back();
      pending.pop_back();
      if (node->index() == index) return true;
      for (const auto& child : node->children) pending.push_back(child.get());
    }
    return false;
  }

  static bool isGeometryLeaf(const AbstractNode& node)
  {
    return dynamic_cast<const AbstractPolyNode *>(&node) ||
           dynamic_cast<const RenderNode *>(&node) || dynamic_cast<const CgalAdvNode *>(&node);
  }
  std::vector<std::shared_ptr<const AbstractNode>> objects;
  int position{-1};
};
