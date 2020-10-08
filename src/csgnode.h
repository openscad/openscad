#pragma once

#include <string>
#include <vector>
#include "memory.h"
#include "Geometry.h"
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
	virtual std::string dump() const = 0;
	virtual bool isEmptySet() { return false; }

	const BoundingBox &getBoundingBox() const { return this->bbox; }
	unsigned int getFlags() const { return this->flags; }
	bool isHighlight() const { return this->flags & FLAG_HIGHLIGHT; }
	bool isBackground() const { return this->flags & FLAG_BACKGROUND; }
	void setHighlight(bool on) { on ? this->flags |= FLAG_HIGHLIGHT : this->flags &= ~FLAG_HIGHLIGHT; }
	void setBackground(bool on) { on ? this->flags |= FLAG_BACKGROUND : this->flags &= ~FLAG_BACKGROUND; }

	static shared_ptr<CSGNode> createEmptySet();

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
	std::string dump() const override;

	shared_ptr<CSGNode> &left() { return this->children[0]; }
	shared_ptr<CSGNode> &right() { return this->children[1]; }
	const shared_ptr<CSGNode> &left() const { return this->children[0]; }
	const shared_ptr<CSGNode> &right() const { return this->children[1]; }

	OpenSCADOperator getType() const { return this->type; }

	static shared_ptr<CSGNode> createCSGNode(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);

private:
	CSGOperation(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right);
	OpenSCADOperator type;
	std::vector<shared_ptr<CSGNode> > children;
};

// very large lists of children can overflow stack due to recursive destruction of shared_ptr,
// so move shared_ptrs into a temporary vector
struct CSGOperationDeleter {
	void operator()(CSGOperation* node) {
		std::vector<shared_ptr<CSGNode>> purge;
		purge.emplace_back(std::move(node->right()));
		purge.emplace_back(std::move(node->left()));
		delete node;
		do {
			auto op = dynamic_pointer_cast<CSGOperation>(purge.back());
		  purge.pop_back();
			if (op && op.use_count() == 1) {
				purge.emplace_back(std::move(op->right()));
				purge.emplace_back(std::move(op->left()));
			}
		} while(!purge.empty());
	}
};

class CSGLeaf : public CSGNode
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	CSGLeaf(const shared_ptr<const class Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label, const int index);
	~CSGLeaf() {}
	void initBoundingBox() override;
	bool isEmptySet() override { return geom == nullptr || geom->isEmpty(); }
	std::string dump() const override;
	std::string label;
	shared_ptr<const Geometry> geom;
	Transform3d matrix;
	Color4f color;

	const int index;

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
