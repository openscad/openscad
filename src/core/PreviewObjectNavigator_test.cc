#include "core/PreviewObjectNavigator.h"

#include <catch2/catch_all.hpp>
#include <memory>

#include "core/CSGTreeEvaluator.h"
#include "core/CsgOpNode.h"
#include "core/TransformNode.h"
#include "core/Tree.h"
#include "core/primitives.h"
#include "geometry/GeometryEvaluator.h"

namespace {

Location sourceLocation(int line)
{
  return {line, 1, line, 10, std::make_shared<fs::path>("/tmp/preview-navigation.scad")};
}

}  // namespace

TEST_CASE("Preview objects follow evaluated tree order and restart after preview", "[preview-navigation]")
{
  ModuleInstantiation transformSource("translate", {}, sourceLocation(1));
  ModuleInstantiation firstSource("cube", {}, sourceLocation(2));
  ModuleInstantiation secondSource("cube", {}, sourceLocation(3));
  ModuleInstantiation echoSource("echo", {}, sourceLocation(4));
  auto root = std::make_shared<RootNode>();
  auto transform = std::make_shared<TransformNode>(&transformSource, "translate");
  auto first = std::make_shared<CubeNode>(&firstSource);
  auto second = std::make_shared<CubeNode>(&secondSource);
  auto synthetic = std::make_shared<GroupNode>(nullptr);
  auto echoOnly = std::make_shared<GroupNode>(&echoSource);
  echoOnly->children = {std::make_shared<GroupNode>(&echoSource)};
  transform->children.push_back(first);
  synthetic->children.push_back(second);
  root->children = {transform, echoOnly, synthetic};

  PreviewObjectNavigator navigation;
  CHECK(navigation.step(1) == nullptr);
  navigation.reset(root);
  REQUIRE(navigation.size() == 2);
  CHECK(navigation.step(1) == first);
  CHECK(navigation.otherObjectIndices() == std::vector<int>{second->index()});
  CHECK(navigation.step(1) == second);
  CHECK(navigation.step(1) == second);
  CHECK(navigation.step(-1) == first);
  CHECK(navigation.step(-1) == first);

  navigation.reset(root);
  CHECK(navigation.step(-1) == second);
  CHECK(navigation.step(1) == second);
  navigation.rewind();
  CHECK(navigation.step(1) == first);
  auto nextPreview = std::make_shared<RootNode>();
  nextPreview->children = {second};
  navigation.reset(nextPreview);
  CHECK(navigation.step(1) == second);
  navigation.clear();
  CHECK(navigation.size() == 0);
  CHECK(navigation.step(-1) == nullptr);
}

TEST_CASE("Preview emphasis selects one evaluated instance without changing source tags", "[preview-navigation]")
{
  ModuleInstantiation transformSource("translate", {}, sourceLocation(1));
  ModuleInstantiation sharedSource("cube", {}, sourceLocation(2));
  auto root = std::make_shared<RootNode>();
  auto transform = std::make_shared<TransformNode>(&transformSource, "translate");
  transform->matrix.translate(Vector3d(12, -3, 4));
  auto first = std::make_shared<CubeNode>(&sharedSource);
  auto second = std::make_shared<CubeNode>(&sharedSource);
  transform->children = {first};
  root->children = {transform, second};
  Tree tree(root);
  GeometryEvaluator geometry(tree);
  const auto original = tree.getString(*root, "");

  CSGTreeEvaluator normal(tree, &geometry);
  normal.buildCSGTree(*root);
  CHECK(normal.getHighlightNodes().empty());

  CSGTreeEvaluator selected(tree, &geometry, first->index());
  selected.buildCSGTree(*root);
  REQUIRE(selected.getHighlightNodes().size() == 1);
  const auto leaf = std::dynamic_pointer_cast<CSGLeaf>(selected.getHighlightNodes().front());
  REQUIRE(leaf);
  CHECK(leaf->index == first->index());
  CHECK(leaf->matrix.translation().isApprox(Vector3d(12, -3, 4)));
  CHECK_FALSE(sharedSource.isHighlight());
  CHECK_FALSE(transformSource.isHighlight());
  CHECK(tree.getString(*root, "") == original);

  CSGTreeEvaluator isolated(tree, &geometry, first->index(), {second->index()});
  isolated.buildCSGTree(*root);
  CHECK(isolated.getBackgroundNodes().size() == 1);

  CSGTreeEvaluator restored(tree, &geometry);
  restored.buildCSGTree(*root);
  CHECK(restored.getHighlightNodes().empty());
}

TEST_CASE("Preview navigation treats evaluated meshes as single objects", "[preview-navigation]")
{
  ModuleInstantiation renderSource("render", {}, sourceLocation(1));
  ModuleInstantiation childSource("cube", {}, sourceLocation(2));
  auto root = std::make_shared<RootNode>();
  auto render = std::make_shared<RenderNode>(&renderSource);
  render->children = {std::make_shared<CubeNode>(&childSource)};
  root->children = {render};
  PreviewObjectNavigator navigation;
  navigation.reset(root);
  CHECK(navigation.size() == 1);
  CHECK(navigation.step(1) == render);
  CHECK(navigation.step(1) == render);
}

TEST_CASE("Preview emphasis preserves a selected boolean subtree and parent transform", "[preview-navigation]")
{
  ModuleInstantiation transformSource("translate", {}, sourceLocation(1));
  ModuleInstantiation differenceSource("difference", {}, sourceLocation(2));
  ModuleInstantiation firstSource("cube", {}, sourceLocation(3));
  ModuleInstantiation secondSource("cube", {}, sourceLocation(4));
  auto root = std::make_shared<RootNode>();
  auto transform = std::make_shared<TransformNode>(&transformSource, "translate");
  transform->matrix.translate(Vector3d(7, 0, 0));
  auto difference = std::make_shared<CsgOpNode>(&differenceSource, OpenSCADOperator::DIFFERENCE);
  auto first = std::make_shared<CubeNode>(&firstSource);
  first->x = first->y = first->z = 3;
  auto second = std::make_shared<CubeNode>(&secondSource);
  difference->children = {first, second};
  transform->children = {difference};
  root->children = {transform};
  Tree tree(root);
  GeometryEvaluator geometry(tree);
  CSGTreeEvaluator selected(tree, &geometry, difference->index());
  selected.buildCSGTree(*root);

  REQUIRE(selected.getHighlightNodes().size() == 1);
  const auto operation = std::dynamic_pointer_cast<CSGOperation>(selected.getHighlightNodes().front());
  REQUIRE(operation);
  CHECK(operation->getType() == OpenSCADOperator::DIFFERENCE);
  const auto left = std::dynamic_pointer_cast<CSGLeaf>(operation->left());
  const auto right = std::dynamic_pointer_cast<CSGLeaf>(operation->right());
  REQUIRE(left);
  REQUIRE(right);
  CHECK(left->matrix.translation().isApprox(Vector3d(7, 0, 0)));
  CHECK(right->matrix.translation().isApprox(Vector3d(7, 0, 0)));
  CHECK_FALSE(differenceSource.isHighlight());
}
