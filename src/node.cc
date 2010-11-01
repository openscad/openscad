/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#include "printutils.h"
#include "node.h"
#include "module.h"
#include "csgterm.h"
#include "progress.h"
#include "polyset.h"
#include "visitor.h"
#include "nodedumper.h"

#include <QRegExp>
#include <sstream>

size_t AbstractNode::idx_counter;

AbstractNode::AbstractNode(const ModuleInstantiation *mi)
{
	modinst = mi;
	idx = idx_counter++;
}

AbstractNode::~AbstractNode()
{
	foreach (AbstractNode *v, children)
		delete v;
}

Response AbstractNode::accept(class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

Response AbstractIntersectionNode::accept(class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

Response AbstractPolyNode::accept(class State &state, Visitor &visitor) const
{
	return visitor.visit(state, *this);
}

std::string AbstractNode::toString() const
{
	return this->name() + "()";
}

std::string AbstractNode::name() const
{
	return "group";
}

std::string AbstractIntersectionNode::toString() const
{
	return this->name() + "()";
}

std::string AbstractIntersectionNode::name() const
{
	return "intersection_for";
}

void AbstractNode::progress_prepare()
{
	foreach (AbstractNode *v, children)
		v->progress_prepare();
	this->progress_mark = ++progress_report_count;
}

void AbstractNode::progress_report() const
{
	progress_update(this, this->progress_mark);
}

std::ostream &operator<<(std::ostream &stream, const QString &str)
{
	stream << str.toStdString();
	return stream;
}

std::ostream &operator<<(std::ostream &stream, const AbstractNode &node)
{
	stream << node.toString();
	return stream;
}
