#ifndef CSGTERM_H_
#define CSGTERM_H_

#include <string>
#include <vector>
#include "memory.h"
#include "linalg.h"

class PolySet;

class CSGTerm
{
public:
	enum type_e {
		TYPE_PRIMITIVE,
		TYPE_UNION,
		TYPE_INTERSECTION,
		TYPE_DIFFERENCE
	};

	static shared_ptr<CSGTerm> createCSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right);
	static shared_ptr<CSGTerm> createCSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	type_e type;
	shared_ptr<PolySet> polyset;
	std::string label;
	shared_ptr<CSGTerm> left;
	shared_ptr<CSGTerm> right;
	BoundingBox bbox;

	CSGTerm(const shared_ptr<PolySet> &polyset, const Transform3d &matrix, const double color[4], const std::string &label);
	~CSGTerm();

	const BoundingBox &getBoundingBox() const { return this->bbox; }

	std::string dump();
private:
	CSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right);
	CSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	void initBoundingBox();

	Transform3d m;
	double color[4];

	friend class CSGChain;
};

class CSGChain
{
public:
	std::vector<shared_ptr<PolySet> > polysets;
	std::vector<Transform3d> matrices;
	std::vector<double*> colors;
	std::vector<CSGTerm::type_e> types;
	std::vector<std::string> labels;

	CSGChain();

	void add(const shared_ptr<PolySet> &polyset, const Transform3d &m, double *color, CSGTerm::type_e type, std::string label);
	void import(shared_ptr<CSGTerm> term, CSGTerm::type_e type = CSGTerm::TYPE_UNION);
	std::string dump();

	BoundingBox getBoundingBox() const;
};

#endif
