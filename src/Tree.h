#ifndef TREE_H_
#define TREE_H_

#include "nodecache.h"
//#include "cgal.h"

using std::string;

class Tree
{
public:
	Tree() {
		this->root_node = NULL;
	}
	~Tree() {}

	void setRoot(const AbstractNode *root) { this->root_node = root; }
	const AbstractNode *root() const { return this->root_node; }

  // FIXME: Really return a reference?
	const string &getString(const AbstractNode &node) const;
//	CGAL_Nef_polyhedron getCGALMesh(const AbstractNode &node) const;

private:
	const AbstractNode *root_node;
  mutable NodeCache nodecache;
};

#endif
