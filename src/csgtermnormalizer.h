#ifndef CSGTERMNORMALIZER_H_
#define CSGTERMNORMALIZER_H_

#include "memory.h"

class CSGTermNormalizer
{
public:
	CSGTermNormalizer() : counter(0) {}
	~CSGTermNormalizer() {}

	shared_ptr<class CSGTerm> normalize(const shared_ptr<CSGTerm> &term, size_t limit);

private:
	shared_ptr<CSGTerm> normalizePass(shared_ptr<CSGTerm> term) ;
	bool normalize_tail(shared_ptr<CSGTerm> &term);
	int count(const shared_ptr<CSGTerm> &term) const;

	int counter;
};

#endif
