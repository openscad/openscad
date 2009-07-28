/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#define INCLUDE_ABSTRACT_NODE_DETAILS

#include "openscad.h"

CSGTerm::CSGTerm(PolySet *polyset, double m[16], QString label)
{
	this->type = PRIMITIVE;
	this->polyset = polyset;
	this->label = label;
	this->left = NULL;
	this->right = NULL;
	for (int i = 0; i < 16; i++)
		this->m[i] = m[i];
	refcounter = 1;
}

CSGTerm::CSGTerm(type_e type, CSGTerm *left, CSGTerm *right)
{
	this->type = type;
	this->polyset = NULL;
	this->left = left;
	this->right = right;
	refcounter = 1;
}

CSGTerm *CSGTerm::normalize()
{
	// This function implements the CSG normalization
	// Reference: Florian Kirsch, Juergen Doeller,
	// OpenCSG: A Library for Image-Based CSG Rendering,
	// University of Potsdam, Hasso-Plattner-Institute, Germany
	// http://www.opencsg.org/data/csg_freenix2005_paper.pdf

	if (type == PRIMITIVE)
		return link();

	CSGTerm *t1, *t2, *x, *y;

	x = left->normalize();
	y = right->normalize();

	if (x != left || y != right) {
		t1 = new CSGTerm(type, x, y);
	} else {
		t1 = link();
		x->unlink();
		y->unlink();
	}

	while (1) {
		t2 = t1->normalize_tail();
		t1->unlink();
		if (t1 == t2)
			break;
		t1 = t2;
	}

	return t1;
}

CSGTerm *CSGTerm::normalize_tail()
{
	CSGTerm *x, *y, *z;

	// Part A: The 'x . (y . z)' expressions

	x = left;
	y = right->left;
	z = right->right;

	// 1.  x - (y + z) -> (x - y) - z
	if (type == DIFFERENCE && right->type == UNION)
		return new CSGTerm(DIFFERENCE, new CSGTerm(DIFFERENCE, x->link(), y->link()), z->link());

	// 2.  x * (y + z) -> (x * y) + (x * z)
	if (type == INTERSECTION && right->type == UNION)
		return new CSGTerm(UNION, new CSGTerm(INTERSECTION, x->link(), y->link()), new CSGTerm(INTERSECTION, x->link(), z->link()));

	// 3.  x - (y * z) -> (x - y) + (x - z)
	if (type == DIFFERENCE && right->type == INTERSECTION)
		return new CSGTerm(UNION, new CSGTerm(DIFFERENCE, x->link(), y->link()), new CSGTerm(DIFFERENCE, x->link(), z->link()));

	// 4.  x * (y * z) -> (x * y) * z
	if (type == INTERSECTION && right->type == INTERSECTION)
		return new CSGTerm(INTERSECTION, new CSGTerm(INTERSECTION, x->link(), y->link()), z->link());

	// 5.  x - (y - z) -> (x - y) + (x * z)
	if (type == DIFFERENCE && right->type == DIFFERENCE)
		return new CSGTerm(UNION, new CSGTerm(DIFFERENCE, x->link(), y->link()), new CSGTerm(INTERSECTION, x->link(), z->link()));

	// 6.  x * (y - z) -> (x * y) - z
	if (type == INTERSECTION && right->type == DIFFERENCE)
		return new CSGTerm(DIFFERENCE, new CSGTerm(INTERSECTION, x->link(), y->link()), z->link());

	// Part B: The '(x . y) . z' expressions

	x = left->left;
	y = left->right;
	z = right;

	// 7. (x - y) * z  -> (x * z) - y
	if (left->type == DIFFERENCE && type == INTERSECTION)
		return new CSGTerm(DIFFERENCE, new CSGTerm(INTERSECTION, x->link(), z->link()), y->link());

	// 8. (x + y) - z  -> (x - z) + (y - z)
	if (left->type == UNION && type == DIFFERENCE)
		return new CSGTerm(UNION, new CSGTerm(DIFFERENCE, x->link(), z->link()), new CSGTerm(DIFFERENCE, y->link(), z->link()));

	// 9. (x + y) * z  -> (x * z) + (y * z)
	if (left->type == UNION && type == INTERSECTION)
		return new CSGTerm(UNION, new CSGTerm(INTERSECTION, x->link(), z->link()), new CSGTerm(INTERSECTION, y->link(), z->link()));

	return link();
}

CSGTerm *CSGTerm::link()
{
	refcounter++;
	return this;
}

void CSGTerm::unlink()
{
	if (--refcounter <= 0) {
		if (polyset)
			polyset->unlink();
		if (left)
			left->unlink();
		if (right)
			right->unlink();
		delete this;
	}
}

QString CSGTerm::dump()
{
	if (type == UNION)
		return QString("(%1 + %2)").arg(left->dump(), right->dump());
	if (type == INTERSECTION)
		return QString("(%1 * %2)").arg(left->dump(), right->dump());
	if (type == DIFFERENCE)
		return QString("(%1 - %2)").arg(left->dump(), right->dump());
	return label;
}

CSGChain::CSGChain()
{
}

void CSGChain::add(PolySet *polyset, double *m, CSGTerm::type_e type, QString label)
{
	polysets.append(polyset);
	matrices.append(m);
	types.append(type);
	labels.append(label);
}

void CSGChain::import(CSGTerm *term, CSGTerm::type_e type)
{
	if (term->type == CSGTerm::PRIMITIVE) {
		add(term->polyset, term->m, type, term->label);
	} else {
		import(term->left, type);
		import(term->right, term->type);
	}
}

QString CSGChain::dump()
{
	QString text;
	for (int i = 0; i < types.size(); i++)
	{
		if (types[i] == CSGTerm::UNION) {
			if (i != 0)
				text += "\n";
			text += "+";
		}
		if (types[i] == CSGTerm::DIFFERENCE)
			text += " -";
		if (types[i] == CSGTerm::INTERSECTION)
			text += " *";
		text += labels[i];
	}
	text += "\n";
	return text;
}

