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

#include "node.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "progress.h"
#include "printutils.h"
#include <functional>
#include <iostream>
#include <algorithm>

size_t AbstractNode::idx_counter;

AbstractNode::AbstractNode(const ModuleInstantiation *mi) : modinst(mi), idx(idx_counter++)
{
}

AbstractNode::~AbstractNode()
{
	std::for_each(this->children.begin(), this->children.end(), std::default_delete<AbstractNode>());
}

std::string AbstractNode::toString() const
{
	return this->name() + "()";
}

std::string GroupNode::name() const
{
	return "group";
}

std::string RootNode::name() const
{
	return "root";
}

std::string AbstractIntersectionNode::toString() const
{
	return this->name() + "()";
}

std::string AbstractIntersectionNode::name() const
{
	// We write intersection here since the module will have to be evaluated
	// before we get here and it will not longer retain the intersection_for parameters
	return "intersection";
}

void AbstractNode::progress_prepare()
{
	std::for_each(this->children.begin(), this->children.end(), std::mem_fun(&AbstractNode::progress_prepare));
	this->progress_mark = ++progress_report_count;
}

void AbstractNode::progress_report() const
{
	progress_update(this, this->progress_mark);
}

std::ostream &operator<<(std::ostream &stream, const AbstractNode &node)
{
	stream << node.toString();
	return stream;
}

// Do we have an explicit root node (! modifier)?
AbstractNode *find_root_tag(AbstractNode *n)
{
	std::vector<AbstractNode *> rootTags;

	std::function<void (AbstractNode *n)> find_root_tags = [&](AbstractNode *n) {
			for (auto v : n->children) {
				if (v->modinst->tag_root) rootTags.push_back(v);
				find_root_tags(v);
			}
		};

	find_root_tags(n);

	if (rootTags.size() == 0) return nullptr;
	if (rootTags.size() > 1) {
		for (const auto &rootTag : rootTags) {
			PRINTB("WARNING: Root Modifier (!) Added At Line%d \n", rootTag->modinst->location().firstLine());
		}
	}
	return rootTags.front();
}
