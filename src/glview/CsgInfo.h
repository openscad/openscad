#pragma once

#include <memory>
#include <vector>

#include "core/CSGNode.h"
#include "core/Tree.h"

/*
   Small helper class for compiling and normalizing node trees into CSG products
 */
class CsgInfo
{
public:
  CsgInfo() = default;
  std::shared_ptr<class CSGProducts> root_products;
  std::shared_ptr<CSGProducts> highlights_products;
  std::shared_ptr<CSGProducts> background_products;

  bool compile_products(const Tree& tree);
};
