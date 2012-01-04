#include "csgtermnormalizer.h"
#include "csgterm.h"
#include "printutils.h"

shared_ptr<CSGTerm> CSGTermNormalizer::normalize(const shared_ptr<CSGTerm> &root)
{
	shared_ptr<CSGTerm> temp = root;
	while (1) {
		shared_ptr<CSGTerm> n = normalizePass(temp);
		if (temp == n) break;
		temp = n;

		int num = count(temp);
#ifdef DEBUG
		PRINTF("Normalize count: %d\n", num);
#endif
		if (num > 5000) {
			PRINTF("WARNING: Normalized tree is growing past 5000 elements. Aborting normalization.\n");
			return root;
		}
	}
	return temp;
}

shared_ptr<CSGTerm> CSGTermNormalizer::normalizePass(shared_ptr<CSGTerm> term)
{
	// This function implements the CSG normalization
  // Reference:
	// Goldfeather, J., Molnar, S., Turk, G., and Fuchs, H. Near
	// Realtime CSG Rendering Using Tree Normalization and Geometric
	// Pruning. IEEE Computer Graphics and Applications, 9(3):20-28,
	// 1989.
  // http://www.cc.gatech.edu/~turk/my_papers/pxpl_csg.pdf

	if (term->type == CSGTerm::TYPE_PRIMITIVE) {
		return term;
	}

	do {
		while (term && normalize_tail(term)) { }
		if (!term || term->type == CSGTerm::TYPE_PRIMITIVE) return term;
		term->left = normalizePass(term->left);
	} while (term->type != CSGTerm::TYPE_UNION &&
					 (term->right->type != CSGTerm::TYPE_PRIMITIVE || term->left->type == CSGTerm::TYPE_UNION));
	term->right = normalizePass(term->right);

	// FIXME: Do we need to take into account any transformation of item here?
	if (!term->right) {
		if (term->type == CSGTerm::TYPE_UNION || term->type == CSGTerm::TYPE_DIFFERENCE) return term->left;
		else return term->right;
	}
	if (!term->left) {
		if (term->type == CSGTerm::TYPE_UNION) return term->right;
		else return term->left;
	}

	return term;
}

bool CSGTermNormalizer::normalize_tail(shared_ptr<CSGTerm> &term)
{
	if (term->type == CSGTerm::TYPE_UNION || term->type == CSGTerm::TYPE_PRIMITIVE) return false;

	// Part A: The 'x . (y . z)' expressions

	shared_ptr<CSGTerm> x = term->left;
	shared_ptr<CSGTerm> y = term->right->left;
	shared_ptr<CSGTerm> z = term->right->right;

	shared_ptr<CSGTerm> result = term;

	// 1.  x - (y + z) -> (x - y) - z
	if (term->type == CSGTerm::TYPE_DIFFERENCE && term->right->type == CSGTerm::TYPE_UNION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, x, y),
												 z);
		return true;
	}
	// 2.  x * (y + z) -> (x * y) + (x * z)
	else if (term->type == CSGTerm::TYPE_INTERSECTION && term->right->type == CSGTerm::TYPE_UNION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, y), 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, z));
		return true;
	}
	// 3.  x - (y * z) -> (x - y) + (x - z)
	else if (term->type == CSGTerm::TYPE_DIFFERENCE && term->right->type == CSGTerm::TYPE_INTERSECTION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, x, y), 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, x, z));
		return true;
	}
	// 4.  x * (y * z) -> (x * y) * z
	else if (term->type == CSGTerm::TYPE_INTERSECTION && term->right->type == CSGTerm::TYPE_INTERSECTION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, y),
												 z);
		return true;
	}
	// 5.  x - (y - z) -> (x - y) + (x * z)
	else if (term->type == CSGTerm::TYPE_DIFFERENCE && term->right->type == CSGTerm::TYPE_DIFFERENCE) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, x, y), 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, z));
		return true;
	}
	// 6.  x * (y - z) -> (x * y) - z
	else if (term->type == CSGTerm::TYPE_INTERSECTION && term->right->type == CSGTerm::TYPE_DIFFERENCE) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, y),
												 z);
		return true;
	}

	// Part B: The '(x . y) . z' expressions

	x = term->left->left;
	y = term->left->right;
	z = term->right;

	// 7. (x - y) * z  -> (x * z) - y
	if (term->left->type == CSGTerm::TYPE_DIFFERENCE && term->type == CSGTerm::TYPE_INTERSECTION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, z), 
												 y);
		return true;
	}
	// 8. (x + y) - z  -> (x - z) + (y - z)
	else if (term->left->type == CSGTerm::TYPE_UNION && term->type == CSGTerm::TYPE_DIFFERENCE) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, x, z), 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_DIFFERENCE, y, z));
		return true;
	}
	// 9. (x + y) * z  -> (x * z) + (y * z)
	else if (term->left->type == CSGTerm::TYPE_UNION && term->type == CSGTerm::TYPE_INTERSECTION) {
		term = CSGTerm::createCSGTerm(CSGTerm::TYPE_UNION, 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, x, z), 
												 CSGTerm::createCSGTerm(CSGTerm::TYPE_INTERSECTION, y, z));
		return true;
	}

	return false;
}

int CSGTermNormalizer::count(const shared_ptr<CSGTerm> &term) const
{
	if (!term) return 0;
	return term->type == CSGTerm::TYPE_PRIMITIVE ? 1 : 0 + count(term->left) + count(term->right);
}
