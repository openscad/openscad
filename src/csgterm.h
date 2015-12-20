#pragma once

#include <string>
#include <vector>
#include "memory.h"
#include "linalg.h"
#include "enums.h"

class CSGNode
{
public:
	enum Flag {
		FLAG_NONE = 0x00,
		FLAG_BACKGROUND = 0x01,
		FLAG_HIGHLIGHT = 0x02
	};


	CSGNode(Flag flags = FLAG_NONE) : flags(flags) {}
	virtual ~CSGNode() {}

	BoundingBox bbox;
	unsigned int flags;

	const BoundingBox &getBoundingBox() const { return this->bbox; }
	bool isHighlight() const { return this->flags & FLAG_HIGHLIGHT; }
	bool isBackground() const { return this->flags & FLAG_BACKGROUND; }
	void setHighlight(bool on) { on ? this->flags |= FLAG_HIGHLIGHT : this->flags &= ~FLAG_HIGHLIGHT; }
	void setBackground(bool on) { on ? this->flags |= FLAG_BACKGROUND : this->flags &= ~FLAG_BACKGROUND; }

	virtual std::string dump() = 0;
protected:
	virtual void initBoundingBox() = 0;

	friend class CSGProducts;
};

class CSGOperation : public CSGNode
{
public:
	CSGOperation() {}
	virtual ~CSGOperation() {}
	virtual void initBoundingBox();
	shared_ptr<CSGNode> &left() {
		return this->children[0];
	}
	shared_ptr<CSGNode> &right() {
		return this->children[1];
	}
	virtual std::string dump();

	static shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);
	static shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, CSGNode *left, CSGNode *right) {
		return createCSGNode(type, shared_ptr<CSGNode>(left), shared_ptr<CSGNode>(right));
	}

	OpenSCADOperator type;
	std::vector<shared_ptr<CSGNode> > children;

private:
	CSGOperation(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);
	CSGOperation(OpenSCADOperator type, CSGNode *left, CSGNode *right);

};

class CSGLeaf : public CSGNode
{
public:
	CSGLeaf(const shared_ptr<const class Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label);
	virtual ~CSGLeaf() {}
	virtual void initBoundingBox();
	virtual std::string dump();

	std::string label;
	shared_ptr<const Geometry> geom;
	Transform3d m;
	Color4f color;

	friend class CSGProducts;
};

class CSGChainObject
{
public:
	CSGChainObject(shared_ptr<const Geometry> geom,
								 const Transform3d &matrix,
								 const Color4f &color,
								 const std::string &label,
								 CSGNode::Flag flags = CSGNode::FLAG_NONE)
		: geom(geom), matrix(matrix), color(color), label(label), flags(flags) {}

	shared_ptr<const Geometry> geom;
	Transform3d matrix;
	Color4f color;
	std::string label;
	CSGNode::Flag flags;
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

	void import(shared_ptr<CSGNode> term, OpenSCADOperator type = OPENSCAD_UNION, CSGNode::Flag flags = CSGNode::FLAG_NONE);
	std::string dump(bool full = false) const;
	BoundingBox getBoundingBox() const;

	std::vector<CSGProduct> products;

	size_t size() const;
	
private:
	void createProduct() {
		this->products.push_back(CSGProduct());
		this->currentproduct = &this->products.back();
    this->currentlist = &this->currentproduct->intersections;
	}

	std::vector<CSGChainObject> *currentlist;
	CSGProduct *currentproduct;
};
