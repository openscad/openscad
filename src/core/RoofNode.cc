// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#include "core/RoofNode.h"

#include <algorithm>
#include <utility>
#include <memory>
#include <sstream>

#include "core/module.h"
#include "core/ModuleInstantiation.h"
#include "core/Builtins.h"
#include "core/Parameters.h"
#include "core/Children.h"

static std::shared_ptr<AbstractNode> builtin_roof(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  auto node = std::make_shared<RoofNode>(inst);

  Parameters parameters = Parameters::parse(std::move(arguments), inst->location(),
                                            {"method"},
                                            {"convexity"}
                                            );

  node->fn = parameters["$fn"].toDouble();
  node->fs = parameters["$fs"].toDouble();
  node->fa = parameters["$fa"].toDouble();

  node->fa = std::max(node->fa, 0.01);
  node->fs = std::max(node->fs, 0.01);
  if (node->fn > 0) {
    node->fa = 360.0 / node->fn;
    node->fs = 0.0;
  }

  if (parameters["method"].isUndefined()) {
    node->method = "voronoi";
  } else {
    node->method = parameters["method"].toString();
    // method can only be one of...
    if (node->method != "voronoi" && node->method != "straight") {
      LOG(message_group::Warning, inst->location(), parameters.documentRoot(),
          "Unknown roof method '" + node->method + "'. Using 'voronoi'.");
      node->method = "voronoi";
    }
  }

  double tmp_convexity = 0.0;
  parameters["convexity"].getFiniteDouble(tmp_convexity);
  node->convexity = static_cast<int>(tmp_convexity);
  if (node->convexity <= 0) node->convexity = 1;

  children.instantiate(node);

  return node;
}

std::string RoofNode::toString() const
{
  std::stringstream stream;

  stream << "roof(method = \"" << this->method << "\""
         << ", $fa = " << this->fa
         << ", $fs = " << this->fs
         << ", $fn = " << this->fn
         << ", convexity = " << this->convexity
         << ")";

  return stream.str();
}

void register_builtin_roof()
{
  Builtins::init("roof", new BuiltinModule(builtin_roof, &Feature::ExperimentalRoof), {
    "roof(method = \"voronoi\")"
  });
}
