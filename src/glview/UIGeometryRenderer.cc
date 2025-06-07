#include "UIGeometryRenderer.h"

#include "glview/PolySetRenderer.h"
#include "core/ColorNode.h"
#include "core/ModuleInstantiation.h"
#include "core/TransformNode.h"
#include "core/Tree.h"
#include "core/primitives.h"
#include "geometry/GeometryEvaluator.h"

UIGeometryRenderer::UIGeometryRenderer(double zoomValue)
  : zoomValue(zoomValue)
  , module(new ModuleInstantiation("UI"))
  , root(new GroupNode(module.get()))
{

}

void UIGeometryRenderer::addPoint(const Vector3d& pt, const Color4f& color)
{
  auto tr = std::make_shared<TransformNode>(module.get(), "multmatrix");
  tr->matrix = Eigen::Translation3d(pt);
  tr->children.push_back(makePointSphereNode());
  root->children.push_back(makeColorNode(tr, color));
}

void UIGeometryRenderer::addLine(const Vector3d& start, const Vector3d& end, const Color4f& color)
{
  double len = (end - start).norm();

  auto tr1 = std::make_shared<TransformNode>(module.get(), "multmatrix");
  tr1->matrix = Eigen::Translation3d(start) * Eigen::Quaterniond::FromTwoVectors(Vector3d::UnitZ(), end - start);

  tr1->children.push_back(makePointSphereNode());

  auto tr2 = std::make_shared<TransformNode>(module.get(), "multmatrix");
  tr2->matrix = Eigen::Translation3d(Vector3d(0.0, 0.0, len));
  tr2->children.push_back(makePointSphereNode());

  tr1->children.push_back(tr2);

  // This magic factor makes the cylinder and sphere radii match up closely
  double r = zoomValue * 0.002 * 0.92;
  auto cyl = std::make_shared<CylinderNode>(module.get());
  cyl->fn = 8;
  cyl->r1 = r;
  cyl->r2 = r;
  cyl->h = len;
  tr1->children.push_back(cyl);

  root->children.push_back(makeColorNode(tr1, color));
}

void UIGeometryRenderer::draw(ShaderUtils::ShaderInfo *shaderInfo)
{
  if (root->children.empty()) {
    return;
  }

  Tree tree(root, "GUI");
  GeometryEvaluator evaluator(tree);
  std::shared_ptr<const Geometry> geometry = evaluator.evaluateGeometry(*tree.root(), true);

  PolySetRenderer renderer(geometry);
  renderer.prepare(shaderInfo);

  // Render the geometry twice, first semi-transparently without depth test, and then in solid color with depth test.
  // This makes UI geometry that is hidden behind the model faintly visible.

  glEnable(GL_CULL_FACE);

  glDisable(GL_DEPTH_TEST);
  glBlendColor(0.0, 0.0, 0.0, 0.15);
  glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  renderer.draw(false, shaderInfo);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);

  renderer.draw(false, shaderInfo);

  glDisable(GL_CULL_FACE);
}

std::shared_ptr<AbstractNode> UIGeometryRenderer::makeColorNode(const std::shared_ptr<AbstractNode>& child, const Color4f& color)
{
  auto node = std::make_shared<ColorNode>(module.get());
  node->color = color;
  node->children.push_back(child);
  return node;
}

std::shared_ptr<AbstractNode> UIGeometryRenderer::makePointSphereNode()
{
  auto node = std::make_shared<SphereNode>(module.get());
  node->fn = 8;
  node->r = zoomValue * 0.002;
  return node;
}