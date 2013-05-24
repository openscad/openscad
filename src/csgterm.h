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

	enum Flag {
		FLAG_NONE = 0x00,
		FLAG_BACKGROUND = 0x01,
		FLAG_HIGHLIGHT = 0x03
	};

	static shared_ptr<CSGTerm> createCSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right);
	static shared_ptr<CSGTerm> createCSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	type_e type;
	shared_ptr<PolySet> polyset;
	std::string label;
	shared_ptr<CSGTerm> left;
	shared_ptr<CSGTerm> right;
	BoundingBox bbox;
	Flag flag;

	CSGTerm(const shared_ptr<PolySet> &polyset, const Transform3d &matrix, const Color4f &color, const std::string &label);
	~CSGTerm();

	const BoundingBox &getBoundingBox() const { return this->bbox; }

	std::string dump();
private:
	CSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right);
	CSGTerm(type_e type, CSGTerm *left, CSGTerm *right);

	void initBoundingBox();

	Transform3d m;
	Color4f color;

	friend class CSGChain;
};

class CSGChainObject
{
public:
	CSGChainObject(shared_ptr<PolySet> polyset,
								 const Transform3d &matrix,
								 const Color4f &color,
								 CSGTerm::type_e type,
								 const std::string &label,
								 CSGTerm::Flag flag = CSGTerm::FLAG_NONE)
		: polyset(polyset), matrix(matrix), color(color), type(type), label(label), flag(flag) {}

	shared_ptr<PolySet> polyset;
	Transform3d matrix;
	Color4f color;
	CSGTerm::type_e type;
	std::string label;
	CSGTerm::Flag flag;
};

class CSGChain
{
public:
	std::vector<CSGChainObject> objects;

	CSGChain() {};

	void import(shared_ptr<CSGTerm> term, CSGTerm::type_e type = CSGTerm::TYPE_UNION,
							CSGTerm::Flag flag = CSGTerm::FLAG_NONE);
	std::string dump(bool full = false);

	BoundingBox getBoundingBox() const;
};

#endif
