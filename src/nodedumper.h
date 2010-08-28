#ifndef NODEDUMPER_H_
#define NODEDUMPER_H_

#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "nodecache.h"

using std::string;
using std::map;
using std::list;

class NodeDumper : public Visitor
{
public:
	/*! If idPrefix is true, we will output "n<id>:" in front of each node, 
		  which is useful for debugging. */
	NodeDumper(NodeCache &cache, bool idPrefix = false) : 
		cache(cache), idprefix(idPrefix), root(NULL) { }
  virtual ~NodeDumper() {}

  virtual Response visit(State &state, const AbstractNode &node);

private:
  void handleVisitedChildren(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node) const;
  void handleIndent(const State &state);
	string dumpChildren(const AbstractNode &node);

  NodeCache &cache;
	bool idprefix;

  string currindent;
  const AbstractNode *root;
  typedef list<const AbstractNode *> ChildList;
  map<int, ChildList> visitedchildren;
};

#endif
