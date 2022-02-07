#include "csgops.h"
#include "colornode.h"
#include "feature.h"
#include "ModuleInstantiation.h"
#include "transformnode.h"
#include "printutils.h"

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

/*! Destructively transforms the tree according to various enabled features.
 * Generates nodes that may lack proper debug info in the process, so this
 * definitely should be considererd highly experimental.
 *
 * This code is quite obviously very hard to maintain: it's a hack, and
 * just a way to evaluate this approach / a discussion starter. We might want
 * to roll out some transformer pattern instead, or maybe embed this logic into
 * some AbstractNode::normalize() method instead.
 * 
 * TODO(ochafik): Skip cacheable nodes to avoid working against the cache.
 */
std::shared_ptr<AbstractNode> rewrite_tree(const std::shared_ptr<AbstractNode> &node)
{
	for (auto it = node->children.begin(); it != node->children.end(); it++) {
		*it = rewrite_tree(*it);
	}

	if (hasSpecialTags(node)) {
		return node;
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

	if (std::dynamic_pointer_cast<ListNode>(node) || 
      std::dynamic_pointer_cast<GroupNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

    return node->children.size() == 1 ? node->children[0] : node;
	}
  else if (auto transform = std::dynamic_pointer_cast<TransformNode>(node)) {
		flattenChildren(node, isFlattenableUnionLikeChild);

		// Push down the transform onto its children.
		// Only doing a meaningful rewrite here if there's more than one child
		// or if the child is a color or transform.
		auto children = node->children;
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

		auto children = node->children;
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
			return listOf(node->modinst, node->children);
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
