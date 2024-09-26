#pragma once

#include <cstring>
#include <memory>
#include <utility>
#include "geometry/linalg.h"
#include "core/node.h"

class State
{

public:
  State(std::shared_ptr<const AbstractNode> parent)
    : parentnode(std::move(parent)) {
    this->matrix_ = Transform3d::Identity();
    this->color_.fill(-1.0f);
  }

  void setPrefix(bool on) { FLAG(this->flags, PREFIX, on); }
  void setPostfix(bool on) { FLAG(this->flags, POSTFIX, on); }
  void setHighlight(bool on) { FLAG(this->flags, HIGHLIGHT, on); }
  void setBackground(bool on) { FLAG(this->flags, BACKGROUND, on); }
  void setNumChildren(unsigned int numc) { this->numchildren = numc; }
  void setParent(const std::shared_ptr<const AbstractNode>& parent) { this->parentnode = parent; }
  void setMatrix(const Transform3d& m) { this->matrix_ = m; }
  void setColor(const Color4f& c) { this->color_ = c; }
  void setPreferNef(bool on) { FLAG(this->flags, PREFERNEF, on); }
  [[nodiscard]] bool preferNef() const { return this->flags & PREFERNEF; }

  [[nodiscard]] bool isPrefix() const { return this->flags & PREFIX; }
  [[nodiscard]] bool isPostfix() const { return this->flags & POSTFIX; }
  [[nodiscard]] bool isHighlight() const { return this->flags & HIGHLIGHT; }
  [[nodiscard]] bool isBackground() const { return this->flags & BACKGROUND; }
  [[nodiscard]] unsigned int numChildren() const { return this->numchildren; }
  [[nodiscard]] std::shared_ptr<const AbstractNode> parent() const { return this->parentnode; }
  [[nodiscard]] const Transform3d& matrix() const { return this->matrix_; }
  [[nodiscard]] const Color4f& color() const { return this->color_; }

private:
  enum StateFlags : unsigned int {
    NONE       = 0x00u,
    PREFIX     = 0x01u,
    POSTFIX    = 0x02u,
    PREFERNEF  = 0x04u,
    HIGHLIGHT  = 0x08u,
    BACKGROUND = 0x10u
  };

  constexpr void FLAG(unsigned int& var, StateFlags flag, bool on) {
    if (on) {
      var |= flag;
    } else {
      var &= ~flag;
    }
  }

  unsigned int flags{NONE};
  std::shared_ptr<const AbstractNode> parentnode;
  unsigned int numchildren{0};

  // Transformation matrix and color. FIXME: Generalize such state variables?
  Transform3d matrix_;
  Color4f color_;
};
