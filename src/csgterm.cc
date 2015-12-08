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

#include "csgterm.h"
#include "Geometry.h"
#include "linalg.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/range/iterator_range.hpp>

/*!
	\class CSGTerm

	A CSGTerm is either a "primitive" or a CSG operation with two
	children terms. A primitive in this context is any Geometry, which
	may or may not have a subtree which is already evaluated (e.g. using
	the render() module).

	Note: To distinguish between geometry evaluated to an empty volume
	and non-geometric nodes (e.g. echo), a NULL CSGTerm is considered a
	non-geometric node, while a CSGTerm with a NULL geometry is
	considered empty geometry. This is important when e.g. establishing
	positive vs. negative volumes using the difference operator.
 */

/*!
	\class CSGChain

	A CSGChain is just a vector of primitives, each having a CSG type associated with it.
	It's created by importing a CSGTerm tree.

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
	: label(label), m(matrix), color(color)
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
	this->bbox = this->m * this->geom->getBoundingBox();
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

void CSGProducts::import(shared_ptr<CSGNode> term, OpenSCADOperator type, CSGNode::Flag flag)
{
	CSGNode::Flag newflag = (CSGNode::Flag)(term->flag | flag);

	if (shared_ptr<CSGLeaf> leaf = dynamic_pointer_cast<CSGLeaf>(term)) {
		if (type == OPENSCAD_UNION && this->currentproduct->intersections.size() > 0) {
			this->createProduct();
		}
		else if (type == OPENSCAD_DIFFERENCE) {
			this->currentlist = &this->currentproduct->subtractions;
		}
		this->currentlist->push_back(CSGChainObject(leaf->geom, leaf->m, leaf->color, leaf->label, newflag));
	} else if (shared_ptr<CSGOperation> op = dynamic_pointer_cast<CSGOperation>(term)) {
		assert(op->left() && op->right());
		import(op->left(), type, newflag);
		import(op->right(), op->type, newflag);
	}
}

std::string CSGProduct::dump(bool full) const
{
	std::stringstream dump;
	dump << this->intersections.front().label;
	BOOST_FOREACH(const CSGChainObject &csgobj,
								boost::make_iterator_range(this->intersections.begin() + 1,
																					 this->intersections.end())) {
		dump << " *" << csgobj.label;
	}
	BOOST_FOREACH(const CSGChainObject &csgobj, this->subtractions) {
		dump << " -" << csgobj.label;
	}

// FIXME:
//		if (full) {
//			dump << " polyset: \n" << obj.geom->dump() << "\n";
//			dump << " matrix: \n" << obj.matrix.matrix() << "\n";
//			dump << " color: \n" << obj.color << "\n";
//		}

	return dump.str();
}

BoundingBox CSGProduct::getBoundingBox() const
{
	BoundingBox bbox;
	BOOST_FOREACH(const CSGChainObject &csgobj, this->intersections) {
		if (csgobj.geom) {
			BoundingBox psbox = csgobj.geom->getBoundingBox();
			// FIXME: Should intersect rather than extend
			if (!psbox.isEmpty()) bbox.extend(csgobj.matrix * psbox);
		}
	}
	return bbox;
}

std::string CSGProducts::dump(bool full) const
{
	std::stringstream dump;

	BOOST_FOREACH(const CSGProduct &product, this->products) {
		dump << "+" << product.dump(full) << "\n";
	}
	return dump.str();
}

BoundingBox CSGProducts::getBoundingBox() const
{
	BoundingBox bbox;
	BOOST_FOREACH(const CSGProduct &product, this->products) {
		bbox.extend(product.getBoundingBox());
	}

/*	BOOST_FOREACH(const CSGChainObject &obj, this->objects) {
		if (obj.type != OPENSCAD_DIFFERENCE) {
			if (obj.geom) {
				BoundingBox psbox = obj.geom->getBoundingBox();
				if (!psbox.isNull()) {
					bbox.extend(obj.matrix * psbox);
				}
			}
		}
		} */
	return bbox;
}

size_t CSGProducts::size() const
{
	size_t count = 0;
	BOOST_FOREACH(const CSGProduct &product, this->products) {
		count += product.intersections.size() + product.subtractions.size();
	}
	return count;
}
