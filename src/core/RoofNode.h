// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <exception>
#include <set>
#include <string>
#include <utility>

#include "core/CurveDiscretizer.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"

class RoofNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RoofNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : AbstractPolyNode(mi), discretizer(std::move(discretizer))
  {
  }
  std::string toString() const override;
  std::string name() const override { return "roof"; }

  CurveDiscretizer discretizer;
  int convexity = 1;
  std::string method;

  static std::set<std::string> knownMethods;

  class roof_exception : public std::exception
  {
  public:
    roof_exception(std::string message) : m(std::move(message)) {}
    std::string message() { return m; }

  private:
    std::string m;
  };
};
