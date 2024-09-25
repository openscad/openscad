#pragma once

#include <utility>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include "geometry/linalg.h"
#include "core/enums.h"

class PolySet;

class CSGNode
{
public:
  enum Flag {
    FLAG_NONE = 0x00,
    FLAG_BACKGROUND = 0x01,
    FLAG_HIGHLIGHT = 0x02
  };

  CSGNode(Flag flags = FLAG_NONE) : flags(flags) {}
  virtual ~CSGNode() = default;
  [[nodiscard]] virtual std::string dump() const = 0;
  [[nodiscard]] virtual bool isEmptySet() const { return false; }

  [[nodiscard]] const BoundingBox& getBoundingBox() const { return this->bbox; }
  [[nodiscard]] unsigned int getFlags() const { return this->flags; }
  [[nodiscard]] bool isHighlight() const { return this->flags & FLAG_HIGHLIGHT; }
  [[nodiscard]] bool isBackground() const { return this->flags & FLAG_BACKGROUND; }
  void setHighlight(bool on) { on ? this->flags |= FLAG_HIGHLIGHT : this->flags &= ~FLAG_HIGHLIGHT; }
  void setBackground(bool on) { on ? this->flags |= FLAG_BACKGROUND : this->flags &= ~FLAG_BACKGROUND; }

  static std::shared_ptr<CSGNode> createEmptySet();

protected:
  virtual void initBoundingBox() = 0;

  BoundingBox bbox;
  unsigned int flags;

  friend class CSGProduct;
  friend class CSGProducts;
};

class CSGOperation : public CSGNode
{
public:
  CSGOperation() = default;
  void initBoundingBox() override;
  [[nodiscard]] std::string dump() const override;

  std::shared_ptr<CSGNode>& left() { return this->children[0]; }
  std::shared_ptr<CSGNode>& right() { return this->children[1]; }
  [[nodiscard]] const std::shared_ptr<CSGNode>& left() const { return this->children[0]; }
  [[nodiscard]] const std::shared_ptr<CSGNode>& right() const { return this->children[1]; }

  [[nodiscard]] OpenSCADOperator getType() const { return this->type; }

  static std::shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, std::shared_ptr<CSGNode> left, std::shared_ptr<CSGNode> right);

private:
  CSGOperation(OpenSCADOperator type, const std::shared_ptr<CSGNode>& left, const std::shared_ptr<CSGNode>& right);
  OpenSCADOperator type;
  std::vector<std::shared_ptr<CSGNode>> children;
};

// very large lists of children can overflow stack due to recursive destruction of shared_ptr,
// so move shared_ptrs into a temporary vector
struct CSGOperationDeleter {
  void operator()(CSGOperation *node) {
    std::vector<std::shared_ptr<CSGNode>> purge;
    purge.emplace_back(std::move(node->right()));
    purge.emplace_back(std::move(node->left()));
    delete node;
    do {
      auto op = std::dynamic_pointer_cast<CSGOperation>(purge.back());
      purge.pop_back();
      if (op && op.use_count() == 1) {
        purge.emplace_back(std::move(op->right()));
        purge.emplace_back(std::move(op->left()));
      }
    } while (!purge.empty());
  }
};

class CSGLeaf : public CSGNode
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  CSGLeaf(const std::shared_ptr<const PolySet>& ps, Transform3d matrix, Color4f color, std::string label, const int index);
  void initBoundingBox() override;
  [[nodiscard]] bool isEmptySet() const override;
  [[nodiscard]] std::string dump() const override;
  std::string label;
  std::shared_ptr<const PolySet> polyset;
  Transform3d matrix;
  Color4f color;

  const int index;

  friend class CSGProducts;
};

/*
   Flags are accumulated in the CSG tree, so the rendered object may
   have different flags than the corresponding leaf node.
 */
class CSGChainObject
{
public:
  CSGChainObject(const std::shared_ptr<CSGLeaf>& leaf, CSGNode::Flag flags = CSGNode::FLAG_NONE)
    : leaf(leaf), flags(flags) {}

  std::shared_ptr<CSGLeaf> leaf;
  CSGNode::Flag flags;
};

class CSGProduct
{
public:
  CSGProduct() = default;

  [[nodiscard]] std::string dump() const;
  [[nodiscard]] BoundingBox getBoundingBox(bool throwntogether = false) const;

  std::vector<CSGChainObject> intersections;
  std::vector<CSGChainObject> subtractions;
};

class CSGProducts
{
public:
  CSGProducts() {
    this->createProduct();
  }

  void import(std::shared_ptr<CSGNode> csgtree, OpenSCADOperator type = OpenSCADOperator::UNION, CSGNode::Flag flags = CSGNode::FLAG_NONE);
  [[nodiscard]] std::string dump() const;
  [[nodiscard]] BoundingBox getBoundingBox(bool throwntogether = false) const;

  std::vector<CSGProduct> products;

  [[nodiscard]] size_t size() const;

private:
  void createProduct() {
    this->products.emplace_back();
    this->currentproduct = &this->products.back();
    this->currentlist = &this->currentproduct->intersections;
  }

  std::vector<CSGChainObject> *currentlist;
  CSGProduct *currentproduct;
};
