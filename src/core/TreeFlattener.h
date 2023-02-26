#pragma once

class Tree;

/**
 * Flattens associative ops (unions, intersections, but not hull)
 * and pushes colors and transforms down.
 *
 * This creates flatter trees, with higher arity in commutative operations
 * and more lazy unions at the top.
 * 
 * Skips special nodes and subtrees which are repeated to avoid defeating caching.
*/
void flattenTree(Tree& tree);