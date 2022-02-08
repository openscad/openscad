#include "csgops.h"
#include "colornode.h"
#include "feature.h"
#include "ModuleInstantiation.h"
#include "transformnode.h"
#include "printutils.h"
#include "cgaladvnode.h"
#include <vector>

bool hasSpecialTags(const std::shared_ptr<AbstractNode> &node)
{
	return node->modinst &&
				 (node->modinst->isBackground() || node->modinst->isHighlight() || node->modinst->isRoot());
}

typedef std::vector<std::shared_ptr<AbstractNode>> ChildList;
typedef std::function<bool(const std::shared_ptr<AbstractNode> &)> NodePredicate;
typedef std::function<bool(const std::shared_ptr<AbstractNode> &)> NodePredicate;

void flattenChildren(const std::shared_ptr<AbstractNode> &node, const NodePredicate &predicate,
										 ChildList &out)
{
	for (auto it = node->children.begin(); it != node->children.end(); ++it) {
		auto &child = *it;
		if (predicate(child)) {
			flattenChildren(child, predicate, out);
		}
		else {
			out.push_back(child);
		}
	}
}

void flattenChildren(const std::shared_ptr<AbstractNode> &list, const NodePredicate &predicate)
{
	ChildList children;
	flattenChildren(list, predicate, children);
	list->children = children;
}

bool isUnionLike(const std::shared_ptr<AbstractNode> &node)
{
	if (auto csg = std::dynamic_pointer_cast<CsgOpNode>(node)) {
		return csg->type == OpenSCADOperator::UNION;
	}
	return std::dynamic_pointer_cast<ListNode>(node) || std::dynamic_pointer_cast<GroupNode>(node) ||
				 std::dynamic_pointer_cast<RootNode>(node);
}

std::shared_ptr<AbstractNode> listOf(const ModuleInstantiation *modinst, const ChildList &children)
{
	if (children.size() == 1) {
		return children[0];
	}
	auto list = std::make_shared<ListNode>(modinst);
	list->children = children;
	return list;
}

/*! TODO(ochafik): Skip cacheable nodes to avoid working against the cache.
 */
std::shared_ptr<AbstractNode> rewrite_tree(const std::shared_ptr<AbstractNode> &node)
{
	for (auto it = node->children.begin(); it != node->children.end(); it++) {
		*it = rewrite_tree(*it);
	}

	if (std::dynamic_pointer_cast<CsgOpNode>(node) || std::dynamic_pointer_cast<CgaladvNode>(node)) {

		// Normalize `csg() { list() { a; b } }` -> `csg() { a; b; }`
		if (node->children.size() == 1) {
			auto child = node->children[0];
			if (std::dynamic_pointer_cast<ListNode>(child) ||
					std::dynamic_pointer_cast<GroupNode>(child)) {
				node->children = child->children;
			}
		}
	}

	if (auto csg = std::dynamic_pointer_cast<CsgOpNode>(node)) {
		auto op = csg->type;

		if (op == OpenSCADOperator::UNION || op == OpenSCADOperator::INTERSECTION) {
			flattenChildren(node, [op](auto &child) {
				if (hasSpecialTags(child)) {
					return false;
				}
				if (auto childCsg = std::dynamic_pointer_cast<CsgOpNode>(child)) {
					return childCsg->type == op;
				}
				return false;
			});
		}

		if (op != OpenSCADOperator::HULL && node->children.size() == 1) {
			return node->children[0];
		}

		return node;
	}

	static auto isFlattenableUnionLikeChild = [](auto &child) {
		return !hasSpecialTags(child) && isUnionLike(child);
	};

	auto &children = node->children;

	if (std::dynamic_pointer_cast<ListNode>(node)) {
		if (children.size() == 1) {
			return children[0];
		}
	}
	else if (std::dynamic_pointer_cast<RootNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

		static auto hasRootMark = [](auto n) { return n && n->modinst && n->modinst->isRoot(); };
		if (std::any_of(children.begin(), children.end(), hasRootMark)) {
			// Drop the children that don't have the root ! mark if any of them has it.
			ChildList roots;  // copy_if only avail. in c++20
			for (auto &child : children) {
				if (hasRootMark(child)) roots.push_back(child);
			}
			children = roots;
    }
    if (children.size() == 1 && (std::dynamic_pointer_cast<GroupNode>(children[0]) || std::dynamic_pointer_cast<GroupNode>(children[0]))) {
      auto childrenCopy = children[0]->children;
      children = childrenCopy;
    }
    return node;
	}
	else if (std::dynamic_pointer_cast<GroupNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

		return children.size() == 1 ? children[0] : node;
	}
	else if (auto transform = std::dynamic_pointer_cast<TransformNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

		// Push down the transform onto its children.
		// Only doing a meaningful rewrite here if there's more than one child
		// or if the child is a color or transform.
		if (children.size() > 1 || std::any_of(children.begin(), children.end(), [](auto child) {
					return std::dynamic_pointer_cast<ColorNode>(child) ||
								 std::dynamic_pointer_cast<TransformNode>(child);
				})) {
			for (auto &child : children) {
				if (auto subTransform = std::dynamic_pointer_cast<TransformNode>(child)) {
					subTransform->matrix = transform->matrix * subTransform->matrix;
				}
				else {
					subTransform = std::make_shared<TransformNode>(node->modinst, transform->verbose_name());
					subTransform->matrix = transform->matrix;
					if (auto color = std::dynamic_pointer_cast<ColorNode>(child)) {
						// Keep color above transform, by convention.
						// ColorNode's children move to TransformNode's children, and the
						// TransformNode becomes the ColorNode's only child.
						subTransform->children = color->children;
						// We call this function recursively to give subTransform a
						// chance to be pushed down on its new children.
						color->children = {rewrite_tree(subTransform)};
						child = color;
					}
					else {
						subTransform->children.push_back(child);
						child = subTransform;
					}
				}
			}
			return listOf(node->modinst, children);
		}
	}
	else if (auto color = std::dynamic_pointer_cast<ColorNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

		if (children.size() > 1 || std::any_of(children.begin(), children.end(), [](auto child) {
					return std::dynamic_pointer_cast<ColorNode>(child);
				})) {
			for (auto &child : color->children) {
				if (auto subColor = std::dynamic_pointer_cast<ColorNode>(child)) {
					subColor->color = color->color;
				}
				else {
					subColor = std::make_shared<ColorNode>(node->modinst);
					subColor->color = color->color;
					subColor->children.push_back(child);
					child = subColor;
				}
			}
			return listOf(node->modinst, children);
		}
	}

	return node;
}

void printTree(const AbstractNode &node, const std::string &indent)
{
	auto hasChildren = node.getChildren().size() > 0;
	LOG(message_group::None, Location::NONE, "", "%1$s",
			(indent + node.toString() + (hasChildren ? " {" : ";")).c_str());
	for (const auto child : node.getChildren()) {
		if (child) printTree(*child, indent + "  ");
	}
	if (hasChildren) LOG(message_group::None, Location::NONE, "", "%1$s", (indent + "}").c_str());
}
