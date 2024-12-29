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

#include "linalg.h"
#include "GeometryUtils.h"
#include "primitives.h"
#include "TransformNode.h"
#include "RotateExtrudeNode.h"
#include "LinearExtrudeNode.h"
#include "PathExtrudeNode.h"
#include "PullNode.h"
#include "DebugNode.h"
#include "WrapNode.h"
#include "OversampleNode.h"
#include "FilletNode.h"
#include "CgalAdvNode.h"
#include "CsgOpNode.h"
#include "ColorNode.h"
#include "RoofNode.h"
#include "RenderNode.h"
#include "SurfaceNode.h"
#include "TextNode.h"
#include "OffsetNode.h"
#include "TextureNode.h"
#include "ProjectionNode.h"
#include "ImportNode.h"

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
NodeCloneFunc(SplineNode)
NodeCloneFunc(TransformNode)
NodeCloneFunc(PullNode)
NodeCloneFunc(DebugNode)
NodeCloneFunc(WrapNode)
NodeCloneFunc(ColorNode)
NodeCloneFunc(OversampleNode)
NodeCloneFunc(FilletNode)
NodeCloneFunc(RotateExtrudeNode)
NodeCloneFunc(LinearExtrudeNode)
NodeCloneFunc(PathExtrudeNode)
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
	NodeCloneUse(SplineNode)
	NodeCloneUse(TransformNode)
	NodeCloneUse(PullNode)
	NodeCloneUse(DebugNode)
	NodeCloneUse(WrapNode)
	NodeCloneUse(ColorNode)
	NodeCloneUse(OversampleNode)
	NodeCloneUse(FilletNode)
	NodeCloneUse(RotateExtrudeNode)
	NodeCloneUse(LinearExtrudeNode)
	NodeCloneUse(PathExtrudeNode)
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

void  AbstractNode::dump_counts(int indent,int use_cnt){
  int i=0;
  auto modinst = this->modinst;
  for(i=0;i<indent;i++) printf(" ");

  printf("%s use =%d mi=%p ",this->name().c_str(), use_cnt, this->modinst);

  printf("(%d/%d/%d) ",this->modinst->tag_highlight, this->modinst->tag_background, this->modinst->tag_root);
  printf("\n");  
  for(const auto &child : this->children) {
    child->dump_counts(indent+1, child.use_count());	  
  }
}

