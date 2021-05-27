#include <stack>

#include "CSGTreeNormalizer.h"
#include "csgnode.h"
#include "printutils.h"

// Helper function to debug normalization bugs
#if 0
static bool validate_tree(const shared_ptr<CSGNode> &node)
{
	if (node->getType() == OpenSCADOperator::PRIMITIVE) return true;
    if (!node->left() || !node->right()) return false;
    if (!validate_tree(node->left())) return false;
    if (!validate_tree(node->right())) return false;
    return true;
}
#endif

/*!
	NB! for e.g. empty intersections, this can normalize a tree to nothing and return nullptr.
*/
shared_ptr<CSGNode> CSGTreeNormalizer::normalize(const shared_ptr<CSGNode> &root)
{
	this->aborted = false;
	this->nodecount = 0;
	shared_ptr<CSGNode> temp = root;
	temp = normalizePass(temp);
	this->rootnode.reset();
	return temp;
}

/*!
	After aborting, a subtree might have become invalidated (nullptr child node)
	since terms can be instantiated multiple times.
	This will search for nullptr children an recursively repair the corresponding
	subtree.
 */
shared_ptr<CSGNode> CSGTreeNormalizer::cleanup_term(shared_ptr<CSGNode> &t)
{
	if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(t)) {
		if (op->left()) op->left() = cleanup_term(op->left());
		if (op->right()) op->right() = cleanup_term(op->right());
		return collapse_null_terms(op);
	}
	return t;
}

static bool isUnion(shared_ptr<CSGNode> node) {
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	return op && op->getType() == OpenSCADOperator::UNION;
}

static bool hasRightNonLeaf(shared_ptr<CSGNode> node) {
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	return op->right() && (dynamic_pointer_cast<CSGLeaf>(op->right()) == nullptr);
}

static bool hasLeftUnion(shared_ptr<CSGNode> node) {
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	return op && isUnion(op->left());
}

shared_ptr<CSGNode> CSGTreeNormalizer::normalizePass(shared_ptr<CSGNode> node)
{
	// This function implements the CSG normalization
	// Reference:
	// Goldfeather, J., Molnar, S., Turk, G., and Fuchs, H. Near
	// Realtime CSG Rendering Using Tree Normalization and Geometric
	// Pruning. IEEE Computer Graphics and Applications, 9(3):20-28,
	// 1989.
	// http://www.cc.gatech.edu/~turk/my_papers/pxpl_csg.pdf

	// Iterative tree traversal used to workaround stack limits for very large inputs.
	// This uses dreaded goto calls but is easily verifiable to be
	// functionally equivalent to the original recursive function, 
	// compared to the previous attempt.
	// See Issue #2883 for problem with previous iterative implementation
	// See Pull Request #2343 for the initial reasons for making this not recursive.

	// stores current node and bool indicating if it was a left or right call;
	typedef std::pair<shared_ptr<CSGOperation>, bool> stackframe_t;
	std::stack<stackframe_t> callstack;
	
entrypoint:
	if (dynamic_pointer_cast<CSGLeaf>(node)) goto return_node;
	do {
		while (node && match_and_replace(node)) {	}
		this->nodecount++;
		if (nodecount > this->limit) {
			LOG(message_group::Warning,Location::NONE,"","Normalized tree is growing past %1$d elements. Aborting normalization.\n",this->limit);
			this->aborted = true;
			return shared_ptr<CSGNode>();
		}
		if (!node || dynamic_pointer_cast<CSGLeaf>(node)) goto return_node;
		goto normalize_left_if_op;
cont_left: ;
	} while (!this->aborted && !isUnion(node) && (hasRightNonLeaf(node) || hasLeftUnion(node)));

	if (!this->aborted) {
		goto normalize_right;
cont_right: ;
	}

	// FIXME: Do we need to take into account any transformation of item here?
	node = collapse_null_terms(node);

	if (this->aborted) {
		if (node) node = cleanup_term(node);
	}

return_node:
	if (callstack.empty()) {
		return node;
	} else {
		stackframe_t frame = callstack.top();
		callstack.pop();
		if (frame.second) { // came from a left call
			frame.first->left() = node;
			node = frame.first;
			goto cont_left;
		} else {            // came from a right call
			frame.first->right() = node;
			node = frame.first;
			goto cont_right;
		}
	}
normalize_left_if_op:
	if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node)) {
		callstack.emplace(op, true);
		node = op->left();
		goto entrypoint;
	}
	goto cont_left;
normalize_right:
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	assert(op);
	callstack.emplace(op, false);
	node = op->right();
	goto entrypoint;
}

shared_ptr<CSGNode> CSGTreeNormalizer::collapse_null_terms(const shared_ptr<CSGNode> &node)
{
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	if (op) {
		if (!op->right()) {
			this->nodecount--;
			if (op->getType() == OpenSCADOperator::UNION || op->getType() == OpenSCADOperator::DIFFERENCE) return op->left();
			else return op->right();
		}
		if (!op->left()) {
			this->nodecount--;
			if (op->getType() == OpenSCADOperator::UNION) return op->right();
			else return op->left();
		}
	}
	return node;
}

bool CSGTreeNormalizer::match_and_replace(shared_ptr<CSGNode> &node)
{
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	if (!op) return false;
	if (op->getType() == OpenSCADOperator::UNION) return false;

	// Part A: The 'x . (y . z)' expressions

	shared_ptr<CSGOperation> rightop = dynamic_pointer_cast<CSGOperation>(op->right());
	if (rightop) {
		shared_ptr<CSGNode> x = op->left();
		shared_ptr<CSGNode> y = rightop->left();
		shared_ptr<CSGNode> z = rightop->right();

		// 1.  x - (y + z) -> (x - y) - z
		if (op->getType() == OpenSCADOperator::DIFFERENCE && rightop->getType() == OpenSCADOperator::UNION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, 
																				 CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, x, y),
																				 z);
			return true;
		}
		// 2.  x * (y + z) -> (x * y) + (x * z)
		else if (op->getType() == OpenSCADOperator::INTERSECTION && rightop->getType() == OpenSCADOperator::UNION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::UNION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, y), 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, z));
			return true;
		}
		// 3.  x - (y * z) -> (x - y) + (x - z)
		else if (op->getType() == OpenSCADOperator::DIFFERENCE && rightop->getType() == OpenSCADOperator::INTERSECTION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::UNION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, x, y), 
																		CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, x, z));
			return true;
		}
		// 4.  x * (y * z) -> (x * y) * z
		else if (op->getType() == OpenSCADOperator::INTERSECTION && rightop->getType() == OpenSCADOperator::INTERSECTION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, y),
																		z);
			return true;
		}
		// 5.  x - (y - z) -> (x - y) + (x * z)
		else if (op->getType() == OpenSCADOperator::DIFFERENCE && rightop->getType() == OpenSCADOperator::DIFFERENCE) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::UNION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, x, y), 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, z));
			return true;
		}
		// 6.  x * (y - z) -> (x * y) - z
		else if (op->getType() == OpenSCADOperator::INTERSECTION && rightop->getType() == OpenSCADOperator::DIFFERENCE) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, y),
																		z);
			return true;
		}
	}

	shared_ptr<CSGOperation> leftop = dynamic_pointer_cast<CSGOperation>(op->left());
	if (leftop) {
		// Part B: The '(x . y) . z' expressions
		shared_ptr<CSGNode> x  = leftop->left();
		shared_ptr<CSGNode> y = leftop->right();
		shared_ptr<CSGNode> z = op->right();
		
		// 7. (x - y) * z  -> (x * z) - y
		if (leftop->getType() == OpenSCADOperator::DIFFERENCE && op->getType() == OpenSCADOperator::INTERSECTION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, z), 
																		y);
			return true;
		}
		// 8. (x + y) - z  -> (x - z) + (y - z)
		else if (leftop->getType() == OpenSCADOperator::UNION && op->getType() == OpenSCADOperator::DIFFERENCE) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::UNION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, x, z), 
																		CSGOperation::createCSGNode(OpenSCADOperator::DIFFERENCE, y, z));
			return true;
		}
		// 9. (x + y) * z  -> (x * z) + (y * z)
		else if (leftop->getType() == OpenSCADOperator::UNION && op->getType() == OpenSCADOperator::INTERSECTION) {
			node = CSGOperation::createCSGNode(OpenSCADOperator::UNION, 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, x, z), 
																		CSGOperation::createCSGNode(OpenSCADOperator::INTERSECTION, y, z));
			return true;
		}
	}
	return false;
}

// Counts all non-leaf nodes
unsigned int CSGTreeNormalizer::count(const shared_ptr<CSGNode> &node) const
{
	if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node)) {
		return 1 + count(op->left()) + count(op->right());
	}
	return 0;
}
