#pragma once

#include "node.h"
#include "linalg.h"

class TransformNode : public AbstractNode
{
public:
  VISITABLE();
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  TransformNode(ModuleInstantiation *mi, std::string verbose_name);
  TransformNode(std::string verbose_name) : TransformNode(new ModuleInstantiation("scale"), verbose_name) { }
  std::string toString() const override;
  std::string name() const override;
  std::string verbose_name() const override;
  static std::shared_ptr<TransformNode> scale(double num);
  static std::shared_ptr<TransformNode> scale(std::vector<double>& vec);
  static std::shared_ptr<TransformNode> translate(std::vector<double>& vec);
  static std::shared_ptr<TransformNode> rotate(double angle);
  static std::shared_ptr<TransformNode> rotate(double angle, std::vector<double>& vec);
  static std::shared_ptr<TransformNode> rotate(std::vector<double>& angle);
  static std::shared_ptr<TransformNode> mirror(std::vector<double>& vec);
  static std::shared_ptr<TransformNode> multmatrix(std::vector<std::vector<double>>& vec);
  Transform3d matrix;

private:
  const std::string _name;
};