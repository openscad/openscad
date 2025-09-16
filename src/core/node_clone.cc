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

#include "geometry/linalg.h"
#include "geometry/GeometryUtils.h"
#include "core/primitives.h"
#include "core/TransformNode.h"
#include "core/RotateExtrudeNode.h"
#include "core/LinearExtrudeNode.h"
#include "core/CgalAdvNode.h"
#include "core/CsgOpNode.h"
#include "core/ColorNode.h"
#include "core/RoofNode.h"
#include "core/RenderNode.h"
#include "core/SurfaceNode.h"
#include "core/TextNode.h"
#include "core/OffsetNode.h"
#include "core/ProjectionNode.h"
#include "core/ImportNode.h"

std::vector<ModuleInstantiation *> modinsts_list;

#define NodeCloneFunc(T)                                                           \
  std::shared_ptr<T> clone_what(const T *node)                                     \
  {                                                                                \
    ModuleInstantiation *inst = new ModuleInstantiation(                           \
      node->modinst->name(), node->modinst->arguments, node->modinst->location()); \
    modinsts_list.push_back(inst);                                                 \
    auto clone = std::make_shared<T>(*node);                                       \
    clone->modinst = inst;                                                         \
    return clone;                                                                  \
  }

#define NodeCloneUse(T)                              \
  {                                                  \
    const T *node = dynamic_cast<const T *>(this);   \
    if ((node) != nullptr) clone = clone_what(node); \
  }
NodeCloneFunc(CubeNode) NodeCloneFunc(SphereNode) NodeCloneFunc(CylinderNode)
  NodeCloneFunc(PolyhedronNode) NodeCloneFunc(SquareNode) NodeCloneFunc(CircleNode)
    NodeCloneFunc(PolygonNode) NodeCloneFunc(TransformNode) NodeCloneFunc(ColorNode)
      NodeCloneFunc(RotateExtrudeNode) NodeCloneFunc(LinearExtrudeNode) NodeCloneFunc(CsgOpNode)
        NodeCloneFunc(CgalAdvNode) NodeCloneFunc(RenderNode) NodeCloneFunc(SurfaceNode)
          NodeCloneFunc(TextNode) NodeCloneFunc(OffsetNode) NodeCloneFunc(ProjectionNode)
            NodeCloneFunc(GroupNode) NodeCloneFunc(ImportNode)
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
              NodeCloneFunc(RoofNode)
#endif

                std::shared_ptr<AbstractNode> AbstractNode::clone(void)
{
  std::shared_ptr<AbstractNode> clone = nullptr;
  NodeCloneUse(CubeNode) NodeCloneUse(SphereNode) NodeCloneUse(CylinderNode) NodeCloneUse(PolyhedronNode)
    NodeCloneUse(SquareNode) NodeCloneUse(CircleNode) NodeCloneUse(PolygonNode)
      NodeCloneUse(TransformNode) NodeCloneUse(ColorNode) NodeCloneUse(RotateExtrudeNode)
        NodeCloneUse(LinearExtrudeNode) NodeCloneUse(CsgOpNode) NodeCloneUse(CgalAdvNode)
          NodeCloneUse(RenderNode) NodeCloneUse(SurfaceNode) NodeCloneUse(TextNode)
            NodeCloneUse(OffsetNode) NodeCloneUse(ProjectionNode) NodeCloneUse(GroupNode)
              NodeCloneUse(ImportNode)
#if defined(ENABLE_EXPERIMENTAL) && defined(ENABLE_CGAL)
                NodeCloneUse(RoofNode)
#endif
                  if (clone != nullptr)
  {
    clone->idx = idx_counter++;
    clone->children.clear();
    for (const auto& child : this->children) {
      clone->children.push_back(child->clone());
    }
    return clone;
  }
  std::cout << "Type not defined for clone :" << typeid(this).name() << "\n\r";
  return std::shared_ptr<AbstractNode>(this);
}
