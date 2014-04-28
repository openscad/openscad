#pragma once

#include "memory.h"

class CSGTermNormalizer
{
public:
	CSGTermNormalizer(size_t limit) : limit(limit) {}
	~CSGTermNormalizer() {}

	shared_ptr<class CSGTerm> normalize(const shared_ptr<CSGTerm> &term);

private:
	shared_ptr<CSGTerm> normalizePass(shared_ptr<CSGTerm> term) ;
	bool match_and_replace(shared_ptr<CSGTerm> &term);
	shared_ptr<CSGTerm> collapse_null_terms(const shared_ptr<CSGTerm> &term);
	shared_ptr<CSGTerm> cleanup_term(shared_ptr<CSGTerm> &t);
	unsigned int count(const shared_ptr<CSGTerm> &term) const;

	bool aborted;
	size_t limit;
	size_t nodecount;
	shared_ptr<class CSGTerm> rootnode;
};
