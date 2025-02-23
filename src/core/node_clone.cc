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

#include "src/geometry/linalg.h"
#include "src/geometry/GeometryUtils.h"
#include "src/core/primitives.h"
#include "src/core/TransformNode.h"
#include "src/core/RotateExtrudeNode.h"
#include "src/core/LinearExtrudeNode.h"
#include "src/core/CgalAdvNode.h"
#include "src/core/CsgOpNode.h"
#include "src/core/ColorNode.h"
#include "src/core/RoofNode.h"
#include "src/core/RenderNode.h"
#include "src/core/SurfaceNode.h"
#include "src/core/TextNode.h"
#include "src/core/OffsetNode.h"
#include "src/core/ProjectionNode.h"
#include "src/core/ImportNode.h"

std::vector<ModuleInstantiation *> modinsts_list;

#define NodeCloneFunc(T) std::shared_ptr<T> clone_what(const T *node) {\
       	ModuleInstantiation *inst = new ModuleInstantiation(node->modinst->name() ,\
	node->modinst->arguments, node->modinst->location());\
	modinsts_list.push_back(inst); \
       	auto clone = std::make_shared<T>(*node);\
       	clone->modinst = inst; \
	return clone;\
}

#define NodeCloneUse(T) { const T *node = dynamic_cast<const T *>(this); if((node) != nullptr) clone=clone_what(node); }
NodeCloneFunc(CubeNode)
NodeCloneFunc(SphereNode)
NodeCloneFunc(CylinderNode)
NodeCloneFunc(PolyhedronNode)
NodeCloneFunc(SquareNode)
NodeCloneFunc(CircleNode)
NodeCloneFunc(PolygonNode)
NodeCloneFunc(TransformNode)
NodeCloneFunc(ColorNode)
NodeCloneFunc(RotateExtrudeNode)
NodeCloneFunc(LinearExtrudeNode)
NodeCloneFunc(CsgOpNode)
NodeCloneFunc(CgalAdvNode)
NodeCloneFunc(RoofNode)
NodeCloneFunc(RenderNode)
NodeCloneFunc(SurfaceNode)
NodeCloneFunc(TextNode)
NodeCloneFunc(OffsetNode)
NodeCloneFunc(ProjectionNode)
NodeCloneFunc(GroupNode)
NodeCloneFunc(ImportNode)

std::shared_ptr<AbstractNode> AbstractNode::clone(void)
{
	std::shared_ptr<AbstractNode> clone=nullptr;
	NodeCloneUse(CubeNode)
	NodeCloneUse(SphereNode)
	NodeCloneUse(CylinderNode)
	NodeCloneUse(PolyhedronNode)
	NodeCloneUse(SquareNode)
	NodeCloneUse(CircleNode)
	NodeCloneUse(PolygonNode)
	NodeCloneUse(TransformNode)
	NodeCloneUse(ColorNode)
	NodeCloneUse(RotateExtrudeNode)
	NodeCloneUse(LinearExtrudeNode)
	NodeCloneUse(CsgOpNode)
	NodeCloneUse(CgalAdvNode)
	NodeCloneUse(RoofNode)
	NodeCloneUse(RenderNode)
	NodeCloneUse(SurfaceNode)
	NodeCloneUse(TextNode)
	NodeCloneUse(OffsetNode)
	NodeCloneUse(ProjectionNode)
	NodeCloneUse(GroupNode)
	NodeCloneUse(ImportNode)
	if(clone != nullptr) {
		clone->idx = idx_counter++;
		clone->children.clear();
		for(const auto &child: this->children) {
			clone->children.push_back(child->clone());
		}
		return clone;
	}
	std::cout << "Type not defined for clone :" << typeid(this).name() << "\n\r";
	return std::shared_ptr<AbstractNode>(this);
}
