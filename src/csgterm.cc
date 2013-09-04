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
#include "polyset.h"
#include "linalg.h"
#include <sstream>
#include <boost/foreach.hpp>

/*!
	\class CSGTerm

	A CSGTerm is either a "primitive" or a CSG operation with two
	children terms. A primitive in this context is any PolySet, which
	may or may not have a subtree which is already evaluated (e.g. using
	the render() module).

 */

/*!
	\class CSGChain

	A CSGChain is just a vector of primitives, each having a CSG type associated with it.
	It's created by importing a CSGTerm tree.

 */


shared_ptr<CSGTerm> CSGTerm::createCSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right)
{
	if (type != TYPE_PRIMITIVE) {
		// In case we're creating a CSG terms from a pruned tree, left/right can be NULL
		if (!right) {
			if (type == TYPE_UNION || type == TYPE_DIFFERENCE) return left;
			else return right;
		}
		if (!left) {
			if (type == TYPE_UNION) return right;
			else return left;
		}
	}

  // Pruning the tree. For details, see:
  // http://www.cc.gatech.edu/~turk/my_papers/pxpl_csg.pdf
	const BoundingBox &leftbox = left->getBoundingBox();
	const BoundingBox &rightbox = right->getBoundingBox();
	Vector3d newmin, newmax;
	if (type == TYPE_INTERSECTION) {
#if EIGEN_WORLD_VERSION == 2
		newmin = leftbox.min().cwise().max( rightbox.min() );
		newmax = leftbox.max().cwise().min( rightbox.max() );
#else
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
#endif
		BoundingBox newbox( newmin, newmax );
		if (newbox.isNull()) {
			return shared_ptr<CSGTerm>(); // Prune entire product
		}
	}
	else if (type == TYPE_DIFFERENCE) {
#if EIGEN_WORLD_VERSION == 2
		newmin = leftbox.min().cwise().max( rightbox.min() );
		newmax = leftbox.max().cwise().min( rightbox.max() );
#else
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
#endif
		BoundingBox newbox( newmin, newmax );
		if (newbox.isNull()) {
			return left; // Prune the negative component
		}
	}

	return shared_ptr<CSGTerm>(new CSGTerm(type, left, right));
}

shared_ptr<CSGTerm> CSGTerm::createCSGTerm(type_e type, CSGTerm *left, CSGTerm *right)
{
	return createCSGTerm(type, shared_ptr<CSGTerm>(left), shared_ptr<CSGTerm>(right));
}

CSGTerm::CSGTerm(const shared_ptr<PolySet> &polyset, const Transform3d &matrix, const Color4f &color, const std::string &label)
	: type(TYPE_PRIMITIVE), polyset(polyset), label(label), flag(CSGTerm::FLAG_NONE), m(matrix), color(color)
{
	initBoundingBox();
}

CSGTerm::CSGTerm(type_e type, shared_ptr<CSGTerm> left, shared_ptr<CSGTerm> right)
	: type(type), left(left), right(right), flag(CSGTerm::FLAG_NONE), m(Transform3d::Identity())
{
	initBoundingBox();
}

CSGTerm::CSGTerm(type_e type, CSGTerm *left, CSGTerm *right)
	: type(type), left(left), right(right), flag(CSGTerm::FLAG_NONE), m(Transform3d::Identity())
{
	initBoundingBox();
}

CSGTerm::~CSGTerm()
{
}

void CSGTerm::initBoundingBox()
{
	if (this->type == TYPE_PRIMITIVE) {
		this->bbox = this->m * this->polyset->getBoundingBox();
	}
	else {
		const BoundingBox &leftbox = this->left->getBoundingBox();
		const BoundingBox &rightbox = this->right->getBoundingBox();
		Vector3d newmin, newmax;
		switch (this->type) {
		case TYPE_UNION:
#if EIGEN_WORLD_VERSION == 2
			newmin = leftbox.min().cwise().min( rightbox.min() );
			newmax = leftbox.max().cwise().max( rightbox.max() );
#else
			newmin = leftbox.min().array().cwiseMin( rightbox.min().array() );
			newmax = leftbox.max().array().cwiseMax( rightbox.max().array() );
#endif
			this->bbox = this->m * BoundingBox( newmin, newmax );
			break;
		case TYPE_INTERSECTION:
#if EIGEN_WORLD_VERSION == 2
			newmin = leftbox.min().cwise().max( rightbox.min() );
			newmax = leftbox.max().cwise().min( rightbox.max() );
#else
			newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
			newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
#endif
			this->bbox = this->m * BoundingBox( newmin, newmax );
			break;
		case TYPE_DIFFERENCE:
			this->bbox = this->m * leftbox;
			break;
		case TYPE_PRIMITIVE:
			break;
		default:
			assert(false);
		}
	}
}

std::string CSGTerm::dump()
{
	std::stringstream dump;

	if (type == TYPE_UNION)
		dump << "(" << left->dump() << " + " << right->dump() << ")";
	else if (type == TYPE_INTERSECTION)
		dump << "(" << left->dump() << " * " << right->dump() << ")";
	else if (type == TYPE_DIFFERENCE)
		dump << "(" << left->dump() << " - " << right->dump() << ")";
	else 
		dump << this->label;

	return dump.str();
}

void CSGChain::import(shared_ptr<CSGTerm> term, CSGTerm::type_e type, CSGTerm::Flag flag)
{
	CSGTerm::Flag newflag = (CSGTerm::Flag)(term->flag | flag);
	if (term->type == CSGTerm::TYPE_PRIMITIVE) {
		this->objects.push_back(CSGChainObject(term->polyset, term->m, term->color, type, term->label, newflag));
	} else {
		assert(term->left && term->right);
		import(term->left, type, newflag);
		import(term->right, term->type, newflag);
	}
}

std::string CSGChain::dump(bool full)
{
	std::stringstream dump;

	BOOST_FOREACH(const CSGChainObject &obj, this->objects) {
		if (obj.type == CSGTerm::TYPE_UNION) {
			if (&obj != &this->objects.front()) dump << "\n";
			dump << "+";
		}
		else if (obj.type == CSGTerm::TYPE_DIFFERENCE)
			dump << " -";
		else if (obj.type == CSGTerm::TYPE_INTERSECTION)
			dump << " *";
		dump << obj.label;
		if (full) {
			dump << " polyset: \n" << obj.polyset->dump() << "\n";
			dump << " matrix: \n" << obj.matrix.matrix() << "\n";
			dump << " color: \n" << obj.color << "\n";
		}
	}
	dump << "\n";
	return dump.str();
}

BoundingBox CSGChain::getBoundingBox() const
{
	BoundingBox bbox;
	BOOST_FOREACH(const CSGChainObject &obj, this->objects) {
		if (obj.type != CSGTerm::TYPE_DIFFERENCE) {
			BoundingBox psbox = obj.polyset->getBoundingBox();
			if (!psbox.isNull()) {
				bbox.extend(obj.matrix * psbox);
			}
		}
	}
	return bbox;
}
