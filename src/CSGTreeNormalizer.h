#pragma once

#include "memory.h"

class CSGTreeNormalizer
{
public:
	CSGTreeNormalizer(size_t limit) : limit(limit) {}
	~CSGTreeNormalizer() {}

	shared_ptr<class CSGNode> normalize(const shared_ptr<CSGNode> &term);

private:
	shared_ptr<CSGNode> normalizePass(shared_ptr<CSGNode> term);
	bool match_and_replace(shared_ptr<class CSGNode> &term);
	shared_ptr<CSGNode> collapse_null_terms(const shared_ptr<CSGNode> &term);
	shared_ptr<CSGNode> cleanup_term(shared_ptr<CSGNode> &t);
	unsigned int count(const shared_ptr<CSGNode> &term) const;

	bool aborted;
	size_t limit;
	size_t nodecount;
	shared_ptr<class CSGNode> rootnode;
};
