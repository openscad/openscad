#include "geometry/linalg.h"

#include <memory>

class AbstractNode;
class ModuleInstantiation;

namespace ShaderUtils
{
struct ShaderInfo;
}

class UIGeometryRenderer
{
public:
  UIGeometryRenderer(double zoomValue);
  void addPoint(const Vector3d& pt, const Color4f& color);
  void addLine(const Vector3d& start, const Vector3d& end, const Color4f& color);
  void draw(ShaderUtils::ShaderInfo* shaderInfo);

private:
  std::shared_ptr<AbstractNode> makeColorNode(const std::shared_ptr<AbstractNode>& child, const Color4f& color);
  std::shared_ptr<AbstractNode> makePointSphereNode();

  double zoomValue;
  std::shared_ptr<ModuleInstantiation> module;
  std::shared_ptr<AbstractNode> root;
};
