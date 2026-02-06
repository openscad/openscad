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

static std::shared_ptr<AbstractNode> builtin_roof(const ModuleInstantiation *inst, Arguments arguments,
                                                  const Children& children)
{
  Parameters parameters =
    Parameters::parse(std::move(arguments), inst->location(), {"method"}, {"convexity"});

  auto node = std::make_shared<RoofNode>(inst, CurveDiscretizer(parameters, inst->location()));

  if (parameters["method"].isUndefined()) {
    node->method = "voronoi";
  } else {
    node->method = parameters["method"].toString();
    if (!RoofNode::knownMethods.count(node->method)) {
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

  stream << "roof(method = \"" << this->method << "\"" << ", " << discretizer
         << ", convexity = " << this->convexity << ")";

  return stream.str();
}

std::set<std::string> RoofNode::knownMethods = {"voronoi", "straight"};

void register_builtin_roof()
{
  Builtins::init("roof", new BuiltinModule(builtin_roof, &Feature::ExperimentalRoof),
                 {"roof(method = \"voronoi\")"});
}
