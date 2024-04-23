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
#include "CgalAdvNode.h"
#include "CsgOpNode.h"
#include "ColorNode.h"
#include "RoofNode.h"
#include "RenderNode.h"
#include "SurfaceNode.h"
#include "TextNode.h"
#include "OffsetNode.h"
#include "ProjectionNode.h"
#include "ImportNode.h"

#define NodeCloneFunc(T) std::shared_ptr<T> clone_what(const T *node) { return std::make_shared<T>(*node); }
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
  if(clone != nullptr) { // Credits to Jordan brown for actual implementation hints
    clone->idx = idx_counter++;
    clone->children.clear();
    for(auto &child: this->children) {
      clone->children.push_back(child->clone());
    }
    return clone;
  }
  std::cout << "Type not defined for clone :" << typeid(this).name() << "\n\r";
  return std::shared_ptr<AbstractNode>(this);
}

