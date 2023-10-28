#pragma once

#include "node.h"
#include "linalg.h"

enum class CgalAdvType {
  MINKOWSKI,
  HULL,
  FILL,
  RESIZE
};

class CgalAdvNode : public AbstractNode
{
public:
  VISITABLE();
  CgalAdvNode(CgalAdvType type) : CgalAdvNode(new ModuleInstantiation("cgal_adv_node"), type) { }
  CgalAdvNode(ModuleInstantiation *mi, CgalAdvType type) : AbstractNode(mi), type(type) {
  }
  std::string toString() const override;
  std::string name() const override;
  static std::shared_ptr<CgalAdvNode> minkowski(int convexity = 1);
  static std::shared_ptr<CgalAdvNode> hull();
  static std::shared_ptr<CgalAdvNode> fill();
  static std::shared_ptr<CgalAdvNode> resize(std::vector<double>& newsize);
  static std::shared_ptr<CgalAdvNode> resize(std::vector<double>& newsize, bool autosize);
  static std::shared_ptr<CgalAdvNode> resize(std::vector<double>& newsize, bool autosize, int convexity);
  static std::shared_ptr<CgalAdvNode> resize(std::vector<double>& newsize, std::vector<bool>& autosize);
  static std::shared_ptr<CgalAdvNode> resize(std::vector<double>& newsize, std::vector<bool>& autosize, int convexity);


  unsigned int convexity{1};
  Vector3d newsize;
  Eigen::Matrix<bool, 3, 1> autosize;
  CgalAdvType type;
};
