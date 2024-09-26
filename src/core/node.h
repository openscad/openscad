#pragma once

#include <ostream>
#include <memory>
#include <cstddef>
#include <utility>
#include <utility>
#include <vector>
#include <string>
#include <deque>

#include "core/BaseVisitable.h"
#include "core/AST.h"
#include "core/ModuleInstantiation.h"

extern int progress_report_count;
extern void (*progress_report_f)(const std::shared_ptr<const AbstractNode>&, void *, int);
extern void *progress_report_vp;

void progress_report_prep(const std::shared_ptr<AbstractNode>& root, void (*f)(const std::shared_ptr<const AbstractNode>& node, void *vp, int mark), void *vp);
void progress_report_fin();

/*!

   The node tree is the result of evaluation of a module instantiation
   tree.  Both the module tree and the node tree are regenerated from
   scratch for each compile.

 */
class AbstractNode : public BaseVisitable, public std::enable_shared_from_this<AbstractNode>
{
  // FIXME: the idx_counter/idx is mostly (only?) for debugging.
  // We can hash on pointer value or smth. else.
  //  -> remove and
  // use smth. else to display node identifier in CSG tree output?
  static size_t idx_counter; // Node instantiation index
public:
  VISITABLE();
  AbstractNode(const ModuleInstantiation *mi);
  virtual std::string toString() const;
  /*! The 'OpenSCAD name' of this node, defaults to classname, but can be
      overloaded to provide specialization for e.g. CSG nodes, primitive nodes etc.
      Used for human-readable output. */
  virtual std::string name() const = 0;
  /*| When a more specific name for user interaction shall be used, such as module names,
      the verbose name shall be overloaded. */
  virtual std::string verbose_name() const { return this->name(); }

  const std::vector<std::shared_ptr<AbstractNode>>& getChildren() const {
    return this->children;
  }
  size_t index() const { return this->idx; }

  static void resetIndexCounter() { idx_counter = 1; }

  // FIXME: Make protected
  std::vector<std::shared_ptr<AbstractNode>> children;
  const ModuleInstantiation *modinst;

  // progress_mark is a running number used for progress indication
  // FIXME: Make all progress handling external, put it in the traverser class?
  int progress_mark{0};
  void progress_prepare();
  void progress_report() const;

  int idx; // Node index (unique per tree)

  std::shared_ptr<const AbstractNode> getNodeByID(int idx, std::deque<std::shared_ptr<const AbstractNode>>& path) const;
};

class AbstractIntersectionNode : public AbstractNode
{
public:
  VISITABLE();
  AbstractIntersectionNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  std::string toString() const override;
  std::string name() const override;
};

class AbstractPolyNode : public AbstractNode
{
public:
  VISITABLE();
  AbstractPolyNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }

  enum class render_mode_e {
    RENDER_CGAL,
    RENDER_OPENCSG
  };
};

/*!
   Used for organizing objects into lists which should not be grouped but merely
   unpacked by the parent node.
 */
class ListNode : public AbstractNode
{
public:
  VISITABLE();
  ListNode(const ModuleInstantiation *mi) : AbstractNode(mi) { }
  std::string name() const override;
};

/*!
   Logically groups objects together. Used as a way of passing
   objects around without having to perform unions on them.
 */
class GroupNode : public AbstractNode
{
public:
  VISITABLE();
  GroupNode(const ModuleInstantiation *mi, std::string name = "") : AbstractNode(mi), _name(std::move(name)) { }
  std::string name() const override;
  std::string verbose_name() const override;
private:
  const std::string _name;
};

/*!
   Only instantiated once, for the top-level file.
 */
class RootNode : public GroupNode
{
public:
  VISITABLE();
  RootNode() : GroupNode(&mi), mi("group") { }
  std::string name() const override;
private:
  ModuleInstantiation mi;
};

class LeafNode : public AbstractPolyNode
{
public:
  VISITABLE();
  LeafNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  virtual std::unique_ptr<const class Geometry> createGeometry() const = 0;
};

std::ostream& operator<<(std::ostream& stream, const AbstractNode& node);
std::shared_ptr<AbstractNode> find_root_tag(const std::shared_ptr<AbstractNode>& node, const Location **nextLocation = nullptr);
