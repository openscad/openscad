#ifndef CSGTERM_H_
#define CSGTERM_H_

#include <string>
#include <vector>
#include "polyset.h"
#include "memory.h"

class CSGTerm
{
public:
	enum type_e {
		TYPE_PRIMITIVE,
		TYPE_UNION,
		TYPE_INTERSECTION,
		TYPE_DIFFERENCE
	};

	type_e type;
	shared_ptr<PolySet> polyset;
	std::string label;
	CSGTerm *left;
	CSGTerm *right;
	double m[16];
	double color[4];
	int refcounter;

	CSGTerm(const shared_ptr<PolySet> &polyset, const double matrix[16], const double color[4], const std::string &label);
	CSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	CSGTerm *normalize();
	CSGTerm *normalize_tail();

	CSGTerm *link();
	void unlink();
	std::string dump();
};

class CSGChain
{
public:
	std::vector<shared_ptr<PolySet> > polysets;
	std::vector<double*> matrices;
	std::vector<double*> colors;
	std::vector<CSGTerm::type_e> types;
	std::vector<std::string> labels;

	CSGChain();

	void add(const shared_ptr<PolySet> &polyset, double *m, double *color, CSGTerm::type_e type, std::string label);
	void import(CSGTerm *term, CSGTerm::type_e type = CSGTerm::TYPE_UNION);
	std::string dump();
	std::string fulldump();

	BoundingBox getBoundingBox() const;
};

#endif
