#include "CSGTreeNormalizer.h"
#include "csgnode.h"
#include "printutils.h"

// Helper function to debug normalization bugs
#if 0
static bool validate_tree(const shared_ptr<CSGNode> &node)
{
	if (node->getType() == OPENSCAD_PRIMITIVE) return true;
    if (!node->left() || !node->right()) return false;
    if (!validate_tree(node->left())) return false;
    if (!validate_tree(node->right())) return false;
    return true;
}
#endif

/*!
	NB! for e.g. empty intersections, this can normalize a tree to nothing and return NULL.
*/
shared_ptr<CSGNode> CSGTreeNormalizer::normalize(const shared_ptr<CSGNode> &root)
{
	this->aborted = false;
	shared_ptr<CSGNode> temp = root;
	while (1) {
		this->rootnode = temp;
		this->nodecount = 0;
		shared_ptr<CSGNode> n = normalizePass(temp);
		if (!n) return n; // If normalized to nothing
		if (temp == n) break;
		temp = n;

		if (this->nodecount > this->limit) {
			PRINTB("WARNING: Normalized tree is growing past %d elements. Aborting normalization.\n", this->limit);
      // Clean up any partially evaluated nodes
			shared_ptr<CSGNode> newroot = root, tmproot;
			while (newroot && newroot != tmproot) {
				tmproot = newroot;
				newroot = collapse_null_terms(tmproot);
			}
			newroot = cleanup_term(newroot);
			return newroot;
		}
	}
	this->rootnode.reset();
	return temp;
}

/*!
	After aborting, a subtree might have become invalidated (NULL child node)
	since terms can be instantiated multiple times.
	This will search for NULL children an recursively repair the corresponding
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
	return op && op->getType() == OPENSCAD_UNION;
}

static bool hasRightLeaf(shared_ptr<CSGNode> node) {
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	return op && dynamic_pointer_cast<CSGLeaf>(op->right());
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

	if (dynamic_pointer_cast<CSGLeaf>(node)) return node;

	do {
		while (node && match_and_replace(node)) {	}
		this->nodecount++;
		if (nodecount > this->limit) {
			PRINTB("WARNING: Normalized tree is growing past %d elements. Aborting normalization.\n", this->limit);
			this->aborted = true;
			return shared_ptr<CSGNode>();
		}
		if (!node) return node;
		if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node)) {
			op->left() = normalizePass(op->left());
		}
	} while (!this->aborted && !isUnion(node) &&
					 (!hasRightLeaf(node) ||
						hasLeftUnion(node)));
	if (!this->aborted) {
		shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
		assert(op);
		op->right() = normalizePass(op->right());
	}

	// FIXME: Do we need to take into account any transformation of item here?
	shared_ptr<CSGNode> t = collapse_null_terms(node);

	if (this->aborted) {
		if (t) t = cleanup_term(t);
	}

	return t;
}

shared_ptr<CSGNode> CSGTreeNormalizer::collapse_null_terms(const shared_ptr<CSGNode> &node)
{
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	if (op) {
		if (!op->right()) {
			if (op->getType() == OPENSCAD_UNION || op->getType() == OPENSCAD_DIFFERENCE) return op->left();
			else return op->right();
		}
		if (!op->left()) {
			if (op->getType() == OPENSCAD_UNION) return op->right();
			else return op->left();
		}
	}
	return node;
}

bool CSGTreeNormalizer::match_and_replace(shared_ptr<CSGNode> &node)
{
	shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(node);
	if (!op) return false;
	if (op->getType() == OPENSCAD_UNION) return false;

	// Part A: The 'x . (y . z)' expressions

	shared_ptr<CSGOperation> rightop = dynamic_pointer_cast<CSGOperation>(op->right());
	if (rightop) {
		shared_ptr<CSGNode> x = op->left();
		shared_ptr<CSGNode> y = rightop->left();
		shared_ptr<CSGNode> z = rightop->right();

		// 1.  x - (y + z) -> (x - y) - z
		if (op->getType() == OPENSCAD_DIFFERENCE && rightop->getType() == OPENSCAD_UNION) {
			node = CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, 
																				 CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, x, y),
																				 z);
			return true;
		}
		// 2.  x * (y + z) -> (x * y) + (x * z)
		else if (op->getType() == OPENSCAD_INTERSECTION && rightop->getType() == OPENSCAD_UNION) {
			node = CSGOperation::createCSGNode(OPENSCAD_UNION, 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, y), 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, z));
			return true;
		}
		// 3.  x - (y * z) -> (x - y) + (x - z)
		else if (op->getType() == OPENSCAD_DIFFERENCE && rightop->getType() == OPENSCAD_INTERSECTION) {
			node = CSGOperation::createCSGNode(OPENSCAD_UNION, 
																		CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, x, y), 
																		CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, x, z));
			return true;
		}
		// 4.  x * (y * z) -> (x * y) * z
		else if (op->getType() == OPENSCAD_INTERSECTION && rightop->getType() == OPENSCAD_INTERSECTION) {
			node = CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, y),
																		z);
			return true;
		}
		// 5.  x - (y - z) -> (x - y) + (x * z)
		else if (op->getType() == OPENSCAD_DIFFERENCE && rightop->getType() == OPENSCAD_DIFFERENCE) {
			node = CSGOperation::createCSGNode(OPENSCAD_UNION, 
																		CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, x, y), 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, z));
			return true;
		}
		// 6.  x * (y - z) -> (x * y) - z
		else if (op->getType() == OPENSCAD_INTERSECTION && rightop->getType() == OPENSCAD_DIFFERENCE) {
			node = CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, y),
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
		if (leftop->getType() == OPENSCAD_DIFFERENCE && op->getType() == OPENSCAD_INTERSECTION) {
			node = CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, z), 
																		y);
			return true;
		}
		// 8. (x + y) - z  -> (x - z) + (y - z)
		else if (leftop->getType() == OPENSCAD_UNION && op->getType() == OPENSCAD_DIFFERENCE) {
			node = CSGOperation::createCSGNode(OPENSCAD_UNION, 
																		CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, x, z), 
																		CSGOperation::createCSGNode(OPENSCAD_DIFFERENCE, y, z));
			return true;
		}
		// 9. (x + y) * z  -> (x * z) + (y * z)
		else if (leftop->getType() == OPENSCAD_UNION && op->getType() == OPENSCAD_INTERSECTION) {
			node = CSGOperation::createCSGNode(OPENSCAD_UNION, 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, x, z), 
																		CSGOperation::createCSGNode(OPENSCAD_INTERSECTION, y, z));
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
