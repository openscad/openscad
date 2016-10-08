/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "csgnode.h"
#include "Geometry.h"
#include "linalg.h"
#include <sstream>
#include <boost/range/iterator_range.hpp>

/*!
	\class CSGNode

	CSG trees consiste of CSGNode instances; either a CSGLeaf node
	containing geometry or a CSGOperation performing a basic CSG
	operation on its operands.

	Note: To distinguish between geometry evaluated to an empty volume
	and non-geometric nodes (e.g. echo), a NULL CSGLeaf is considered a
	non-geometric node, while a CSGLeaf with a NULL geometry is
	considered empty geometry. This is important when e.g. establishing
	positive vs. negative volumes using the difference operator.
 */

/*!
	\class CSGProducts

	A CSGProducts is just a vector of CSGProduct nodes.
primitives, each having a CSG type associated with it.
	It's created by importing a CSGTerm tree.

 */
/*!
	\class CSGProduct

	A CSGProduct is a vector of intersections and a vector of subtractions, used for CSG rendering.
*/


shared_ptr<CSGNode> CSGOperation::createCSGNode(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right)
{
	// In case we're creating a CSG terms from a pruned tree, left/right can be NULL
	if (!right) {
		if (type == OPENSCAD_UNION || type == OPENSCAD_DIFFERENCE) return left;
		else return right;
	}
	if (!left) {
		if (type == OPENSCAD_UNION) return right;
		else return left;
	}

  // Pruning the tree. For details, see "Solid Modeling" by Goldfeather:
  // http://www.cc.gatech.edu/~turk/my_papers/pxpl_csg.pdf
	const BoundingBox &leftbox = left->getBoundingBox();
	const BoundingBox &rightbox = right->getBoundingBox();
	Vector3d newmin, newmax;
	if (type == OPENSCAD_INTERSECTION) {
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		BoundingBox newbox( newmin, newmax );
		if (newbox.isNull()) {
			return shared_ptr<CSGNode>(); // Prune entire product
		}
	}
	else if (type == OPENSCAD_DIFFERENCE) {
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		BoundingBox newbox( newmin, newmax );
		if (newbox.isNull()) {
			return left; // Prune the negative component
		}
	}

	return shared_ptr<CSGNode>(new CSGOperation(type, left, right));
}

CSGLeaf::CSGLeaf(const shared_ptr<const Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label)
	: label(label), matrix(matrix), color(color)
{
	if (geom && !geom->isEmpty()) this->geom = geom;
	initBoundingBox();
}

CSGOperation::CSGOperation(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right)
	: type(type)
{
	this->children.push_back(left);
	this->children.push_back(right);
	initBoundingBox();
}

CSGOperation::CSGOperation(OpenSCADOperator type, CSGNode *left, CSGNode *right)
	: type(type)
{
	this->children.push_back(shared_ptr<CSGNode>(left));
	this->children.push_back(shared_ptr<CSGNode>(right));
	initBoundingBox();
}

void CSGLeaf::initBoundingBox()
{
	if (!this->geom) return;
	this->bbox = this->matrix * this->geom->getBoundingBox();
}

void CSGOperation::initBoundingBox()
{
	const BoundingBox &leftbox = this->left()->getBoundingBox();
	const BoundingBox &rightbox = this->right()->getBoundingBox();
	Vector3d newmin, newmax;
	switch (this->type) {
	case OPENSCAD_UNION:
		newmin = leftbox.min().array().cwiseMin( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMax( rightbox.max().array() );
		this->bbox = BoundingBox( newmin, newmax );
		break;
	case OPENSCAD_INTERSECTION:
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		this->bbox = BoundingBox( newmin, newmax );
		break;
	case OPENSCAD_DIFFERENCE:
		this->bbox = leftbox;
		break;
	default:
		assert(false);
	}
}

std::string CSGLeaf::dump()
{
	return this->label;
}

std::string CSGOperation::dump()
{
	std::stringstream dump;

	if (type == OPENSCAD_UNION)
		dump << "(" << left()->dump() << " + " << right()->dump() << ")";
	else if (type == OPENSCAD_INTERSECTION)
		dump << "(" << left()->dump() << " * " << right()->dump() << ")";
	else if (type == OPENSCAD_DIFFERENCE)
		dump << "(" << left()->dump() << " - " << right()->dump() << ")";
	else 
		assert(false);

	return dump.str();
}

void CSGProducts::import(shared_ptr<CSGNode> csgnode, OpenSCADOperator type, CSGNode::Flag flags)
{
	CSGNode::Flag newflags = (CSGNode::Flag)(csgnode->getFlags() | flags);

	if (shared_ptr<CSGLeaf> leaf = dynamic_pointer_cast<CSGLeaf>(csgnode)) {
		if (type == OPENSCAD_UNION && this->currentproduct->intersections.size() > 0) {
			this->createProduct();
		}
		else if (type == OPENSCAD_DIFFERENCE) {
			this->currentlist = &this->currentproduct->subtractions;
		}
		else if (type == OPENSCAD_INTERSECTION) {
			this->currentlist = &this->currentproduct->intersections;
		}
		this->currentlist->push_back(CSGChainObject(leaf, newflags));
	} else if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(csgnode)) {
		assert(op->left() && op->right());
		import(op->left(), type, newflags);
		import(op->right(), op->getType(), newflags);
	}
}

std::string CSGProduct::dump() const
{
	std::stringstream dump;
	dump << this->intersections.front().leaf->label;
	for(const auto &csgobj :
								boost::make_iterator_range(this->intersections.begin() + 1,
																					 this->intersections.end())) {
		dump << " *" << csgobj.leaf->label;
	}
	for(const auto &csgobj : this->subtractions) {
		dump << " -" << csgobj.leaf->label;
	}
	return dump.str();
}

BoundingBox CSGProduct::getBoundingBox() const
{
	BoundingBox bbox;
	for(const auto &csgobj : this->intersections) {
		if (csgobj.leaf->geom) {
			BoundingBox psbox = csgobj.leaf->geom->getBoundingBox();
			// FIXME: Should intersect rather than extend
			if (!psbox.isEmpty()) bbox.extend(csgobj.leaf->matrix * psbox);
		}
	}
	return bbox;
}

std::string CSGProducts::dump() const
{
	std::stringstream dump;

	for(const auto &product : this->products) {
		dump << "+" << product.dump() << "\n";
	}
	return dump.str();
}

BoundingBox CSGProducts::getBoundingBox() const
{
	BoundingBox bbox;
	for(const auto &product : this->products) {
		bbox.extend(product.getBoundingBox());
	}
	return bbox;
}

size_t CSGProducts::size() const
{
	size_t count = 0;
	for(const auto &product : this->products) {
		count += product.intersections.size() + product.subtractions.size();
	}
	return count;
}
