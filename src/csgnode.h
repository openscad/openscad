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
	virtual std::string dump() = 0;

	const BoundingBox &getBoundingBox() const { return this->bbox; }
	unsigned int getFlags() const { return this->flags; }
	bool isHighlight() const { return this->flags & FLAG_HIGHLIGHT; }
	bool isBackground() const { return this->flags & FLAG_BACKGROUND; }
	void setHighlight(bool on) { on ? this->flags |= FLAG_HIGHLIGHT : this->flags &= ~FLAG_HIGHLIGHT; }
	void setBackground(bool on) { on ? this->flags |= FLAG_BACKGROUND : this->flags &= ~FLAG_BACKGROUND; }

protected:
	virtual void initBoundingBox() = 0;

	BoundingBox bbox;
	unsigned int flags;

	friend class CSGProducts;
};

class CSGOperation : public CSGNode
{
public:
	CSGOperation() {}
	~CSGOperation() {}
	void initBoundingBox() override;
	std::string dump() override;

	shared_ptr<CSGNode> &left() { return this->children[0]; }
	shared_ptr<CSGNode> &right() { return this->children[1]; }

	OpenSCADOperator getType() const { return this->type; }
	
	static shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);
	static shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, CSGNode *left, CSGNode *right) {
		return createCSGNode(type, shared_ptr<CSGNode>(left), shared_ptr<CSGNode>(right));
	}

private:
	CSGOperation(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);
	CSGOperation(OpenSCADOperator type, CSGNode *left, CSGNode *right);

	OpenSCADOperator type;
	std::vector<shared_ptr<CSGNode> > children;
};

class CSGLeaf : public CSGNode
{
public:
	CSGLeaf(const shared_ptr<const class Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label);
	~CSGLeaf() {}
	void initBoundingBox() override;
	std::string dump() override;

	std::string label;
	shared_ptr<const Geometry> geom;
	Transform3d matrix;
	Color4f color;

	friend class CSGProducts;
};

/*
	Flags are accumulated in the CSG tree, so the rendered object may
	have different flags than the corresponding leaf node.
*/
class CSGChainObject
{
public:
	CSGChainObject(const shared_ptr<CSGLeaf> &leaf, CSGNode::Flag flags = CSGNode::FLAG_NONE)
		: leaf(leaf), flags(flags) {}

	shared_ptr<CSGLeaf> leaf;
	CSGNode::Flag flags;
};

class CSGProduct
{
public:
	CSGProduct() {}
	~CSGProduct() {}

	std::string dump() const;
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

	void import(shared_ptr<CSGNode> csgtree, OpenSCADOperator type = OpenSCADOperator::UNION, CSGNode::Flag flags = CSGNode::FLAG_NONE);
	std::string dump() const;
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
