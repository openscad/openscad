#pragma once

#include <string>
#include <vector>
#include "memory.h"
#include "linalg.h"

class Geometry;

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
	shared_ptr<const Geometry> geom;
	std::string label;
	shared_ptr<CSGTerm> left;
	shared_ptr<CSGTerm> right;
	BoundingBox bbox;
	Flag flag;

	CSGTerm(const shared_ptr<const Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label);
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
	friend class CSGProducts;
};

class CSGChainObject
{
public:
	CSGChainObject(shared_ptr<const Geometry> geom,
								 const Transform3d &matrix,
								 const Color4f &color,
								 CSGTerm::type_e type,
								 const std::string &label,
								 CSGTerm::Flag flag = CSGTerm::FLAG_NONE)
		: geom(geom), matrix(matrix), color(color), type(type), label(label), flag(flag) {}

	shared_ptr<const Geometry> geom;
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

class CSGProduct
{
public:
	CSGProduct() {}
	~CSGProduct() {}

	std::string dump(bool full = false) const;
	BoundingBox getBoundingBox() const;

	std::vector<CSGChainObject> intersections;
	std::vector<CSGChainObject> subtractions;
};

class CSGProducts
{
public:
	CSGProducts() {
    this->createProduct();
	}
	~CSGProducts() {}

	void import(shared_ptr<CSGTerm> term, CSGTerm::type_e type = CSGTerm::TYPE_UNION, CSGTerm::Flag flag = CSGTerm::FLAG_NONE);
	std::string dump(bool full = false) const;
	BoundingBox getBoundingBox() const;

	std::vector<CSGProduct> products;

private:
	void createProduct() {
		this->products.push_back(CSGProduct());
		this->currentproduct = &this->products.back();
    this->currentlist = &this->currentproduct->intersections;
	}

	std::vector<CSGChainObject> *currentlist;
	CSGProduct *currentproduct;
};
