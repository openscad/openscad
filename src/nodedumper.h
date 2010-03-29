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
	NodeDumper() : root(NULL) {}
  virtual ~NodeDumper() {}

  virtual Response visit(const State &state, const AbstractNode &node);

  const string &getDump() const;
	const NodeCache<string> &getCache() const { return this->cache; }
	void clearCache() { this->cache.clear(); }

	// FIXME: Questionable design...
	static NodeDumper *dumper() { return global_dumper; }
	static void setDumper(NodeDumper *d) { global_dumper = d; }
private:
  void handleVisitedChildren(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node);
  void handleIndent(const State &state);
	string dumpChildren(const AbstractNode &node);

  string currindent;
  const AbstractNode *root;
  typedef list<const AbstractNode *> ChildList;
  map<int, ChildList> visitedchildren;
  NodeCache<string> cache;

	static NodeDumper *global_dumper;
};

#endif
