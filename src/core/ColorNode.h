#pragma once

#include "node.h"
#include "linalg.h"

class ColorNode : public AbstractNode
{
public:
  VISITABLE();
  ColorNode() : ColorNode(new ModuleInstantiation("color")) {}
  ColorNode(Color4f *color);
  ColorNode(std::string colorname, float alpha = 1.0f);
  ColorNode(int red, int green, int blue, int alpha = 255);
  ColorNode(float red, float green, float blue, float alpha = 1.0f);
  ColorNode(ModuleInstantiation *mi) : AbstractNode(mi), color(-1.0f, -1.0f, -1.0f, 1.0f) { }
  
  std::string toString() const override;
  std::string name() const override;

  // static std::shared_ptr<ColorNode> color_() { return std::make_shared<ColorNode>(); }
  // static std::shared_ptr<ColorNode> color_(Color4f *color) { 
  //   return std::make_shared<ColorNode>(color); 
  // }
  // static std::shared_ptr<ColorNode> color_(std::string colorname) { 
  //   return std::make_shared<ColorNode>(colorname); 
  // }
  // static std::shared_ptr<ColorNode> color_(std::string colorname, float alpha) { 
  //   return std::make_shared<ColorNode>(colorname, alpha); 
  // }
  // static std::shared_ptr<ColorNode> color_(int red, int green, int blue) { 
  //   return std::make_shared<ColorNode>(red, green, blue); 
  // }
  // static std::shared_ptr<ColorNode> color_(int red, int green, int blue, int alpha) { 
  //   return std::make_shared<ColorNode>(red, green, blue, alpha); 
  // }
  // static std::shared_ptr<ColorNode> color_(float red, float green, float blue) { 
  //   return std::make_shared<ColorNode>(red, green, blue); 
  // }
  // static std::shared_ptr<ColorNode> color_(float red, float green, float blue, float alpha) { 
  //   return std::make_shared<ColorNode>(red, green, blue, alpha); 
  // }

  static std::list<std::string> getWebColors();

  Color4f color;
};
