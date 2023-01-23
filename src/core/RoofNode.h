// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

#pragma once

#include <string>
#include <utility>

#include "node.h"

class RoofNode : public AbstractPolyNode
{
public:
  VISITABLE();
  RoofNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "roof"; }

  double fa, fs, fn;
  int convexity = 1;
  std::string method;

  class roof_exception : public std::exception
  {
public:
    roof_exception(std::string message) : m(std::move(message)) {}
    std::string message() {return m;}
private:
    std::string m;
  };
};
