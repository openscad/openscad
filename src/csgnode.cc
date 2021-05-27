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
#include "printutils.h"

#include <sstream>
#include <stack>
#include <tuple>

#include <boost/range/iterator_range.hpp>

/*!
	\class CSGNode

	CSG trees consiste of CSGNode instances; either a CSGLeaf node
	containing geometry or a CSGOperation performing a basic CSG
	operation on its operands.

	Note: To distinguish between geometry evaluated to an empty volume
	and non-geometric nodes (e.g. echo), a nullptr CSGLeaf is considered a
	non-geometric node, while a CSGLeaf with a nullptr geometry is
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

shared_ptr<CSGNode> CSGNode::createEmptySet() {
	return shared_ptr<CSGNode>(new CSGLeaf(nullptr, Transform3d(), Color4f(), "empty()", 0));
}

shared_ptr<CSGNode> CSGOperation::createCSGNode(OpenSCADOperator type, shared_ptr<CSGNode> left, shared_ptr<CSGNode> right)
{
	// Note that shared_ptr<CSGNode> == nullptr is different from having a CSGNode with shared_ptr<Geometry> geom == nullptr
	// The former indicates lack of a geometry node (could be echo or assert node), and the latter represents the empty set of geometry.
	if (!left && !right) {
		return CSGNode::createEmptySet();
	} else if (!left && right) {
		return right;
	} else if (left && !right) {
		return left;
	} else {
		// In case we're creating a CSG term from a pruned tree, left or right may be the empty set
		if (right->isEmptySet()) {
		if (type == OpenSCADOperator::UNION || type == OpenSCADOperator::DIFFERENCE) return left;
		else return right;
	}
		if (left->isEmptySet()) {
		if (type == OpenSCADOperator::UNION) return right;
		else return left;
	}
	}

  // Pruning the tree. For details, see "Solid Modeling" by Goldfeather:
  // http://www.cc.gatech.edu/~turk/my_papers/pxpl_csg.pdf
	const auto &leftbox = left->getBoundingBox();
	const auto &rightbox = right->getBoundingBox();
	Vector3d newmin, newmax;
	if (type == OpenSCADOperator::INTERSECTION) {
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		BoundingBox newbox(newmin, newmax);
		if (newbox.isNull()) {
			return CSGNode::createEmptySet(); // Prune entire product
		}
	}
	else if (type == OpenSCADOperator::DIFFERENCE) {
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		BoundingBox newbox( newmin, newmax );
		if (newbox.isNull()) {
			return left; // Prune the negative component
		}
	}

	return shared_ptr<CSGNode>(new CSGOperation(type, left, right), CSGOperationDeleter());
}

CSGLeaf::CSGLeaf(const shared_ptr<const Geometry> &geom, const Transform3d &matrix, const Color4f &color, const std::string &label, const int index)
	: label(label), matrix(matrix), color(color), index(index)
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

void CSGLeaf::initBoundingBox()
{
	if (!this->geom) return;
	this->bbox = this->matrix * this->geom->getBoundingBox();
}

void CSGOperation::initBoundingBox()
{
	const auto &leftbox = this->left()->getBoundingBox();
	const auto &rightbox = this->right()->getBoundingBox();
	Vector3d newmin, newmax;
	switch (this->type) {
	case OpenSCADOperator::UNION:
		newmin = leftbox.min().array().cwiseMin( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMax( rightbox.max().array() );
		this->bbox = BoundingBox( newmin, newmax );
		break;
	case OpenSCADOperator::INTERSECTION:
		newmin = leftbox.min().array().cwiseMax( rightbox.min().array() );
		newmax = leftbox.max().array().cwiseMin( rightbox.max().array() );
		this->bbox = BoundingBox( newmin, newmax );
		break;
	case OpenSCADOperator::DIFFERENCE:
		this->bbox = leftbox;
		break;
	default:
		assert(false);
	}
}

std::string CSGLeaf::dump() const
{
	return this->label;
}

// Recursive traversal can cause stack overflow with very large loops of child nodes,
// so tree is traverse iteratively, managing our own stack.
std::string CSGOperation::dump() const
{
	// tuple(node pointer, postfix string, ispostfix bool)
	std::stack<std::tuple<const CSGOperation*, std::string, bool>> callstack;
	callstack.emplace(this, "", false);
  std::ostringstream out;
	const CSGOperation* node;
	std::string postfixstr;
	bool ispostfix;
  do {
		std::tie(node, postfixstr, ispostfix) = callstack.top();
		if (!ispostfix) { // handle left child. only right child uses a prefix string
			std::string lpostfix;
			switch (node->type) {
			case OpenSCADOperator::UNION:
				lpostfix = " + ";
				break;
			case OpenSCADOperator::INTERSECTION:
				lpostfix = " * ";
				break;
			case OpenSCADOperator::DIFFERENCE:
				lpostfix = " - ";
				break;
			default:
				assert(false);
			}

			out << '(';

			// mark current node as postfix before (maybe) pushing left child
			ispostfix = std::get<2>(callstack.top()) = true;

			if(auto opl = dynamic_pointer_cast<CSGOperation>(node->left())) {
				callstack.emplace(opl.get(), lpostfix, false);
				continue;
			} else {
				out << node->left()->dump() << lpostfix;
			}
		}

		// postfix traversal of node, handle right child
		if (ispostfix) {
			callstack.pop();
			if(auto opr = dynamic_pointer_cast<CSGOperation>(node->right())) {
				callstack.emplace(opr.get(), ")", false);
				continue;
			} else {
				out << node->right()->dump() << ")";
			}
			out << postfixstr;
		}

	} while(!callstack.empty());
	return out.str();
}

void CSGProducts::import(shared_ptr<CSGNode> csgnode, OpenSCADOperator type, CSGNode::Flag flags)
{
	std::stack<std::tuple<shared_ptr<CSGNode>, OpenSCADOperator, CSGNode::Flag>> callstack;
	callstack.push(std::make_tuple(csgnode, type, flags));

	do {
		auto args = callstack.top();
		callstack.pop();
		csgnode = std::get<0>(args);
		type = std::get<1>(args);
		flags = std::get<2>(args);

		auto newflags = static_cast<CSGNode::Flag>(csgnode->getFlags() | flags);

		if (auto leaf = dynamic_pointer_cast<CSGLeaf>(csgnode)) {
			if (type == OpenSCADOperator::UNION && this->currentproduct->intersections.size() > 0) {
				this->createProduct();
			}
			else if (type == OpenSCADOperator::DIFFERENCE) {
				this->currentlist = &this->currentproduct->subtractions;
			}
			else if (type == OpenSCADOperator::INTERSECTION) {
				this->currentlist = &this->currentproduct->intersections;
			}
			this->currentlist->emplace_back(leaf, newflags);
		} else if (auto op = dynamic_pointer_cast<CSGOperation>(csgnode)) {
			assert(op->left() && op->right());
			callstack.emplace(op->right(), op->getType(), newflags);
			callstack.emplace(op->left(), type, newflags);
		}
	} while(!callstack.empty());
}

std::string CSGProduct::dump() const
{
	std::ostringstream dump;
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
			auto psbox = csgobj.leaf->geom->getBoundingBox();
			// FIXME: Should intersect rather than extend
			if (!psbox.isEmpty()) bbox.extend(csgobj.leaf->matrix * psbox);
		}
	}
	return bbox;
}

std::string CSGProducts::dump() const
{
	std::ostringstream dump;

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
