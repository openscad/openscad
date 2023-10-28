#pragma once

#include "node.h"
#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
  VISITABLE();
  TextNode() : TextNode(new ModuleInstantiation("text")) {}
  TextNode(ModuleInstantiation *mi) : AbstractPolyNode(mi) {}

  TextNode(FreetypeRenderer::Params& params);
  TextNode(std::string& text);
  TextNode(std::string& text, int size);
  TextNode(std::string& text, int size, std::string& font);
  // TextNode(std::string& text, std::unordered_map<std::string, std::string>& opts = nullptr);
  // TextNode(std::string& text, int size, std::unordered_map<std::string, std::string>& opts = nullptr);
  // TextNode(std::string& text, std::string& font, std::unordered_map<std::string, std::string>& opts = nullptr);
  // TextNode(std::string& text, int size, std::string& font, std::unordered_map<std::string, std::string>& opts = nullptr);
  static std::shared_ptr<TextNode> text() { return std::make_shared<TextNode>(); }
  static std::shared_ptr<TextNode> text(FreetypeRenderer::Params& params) { 
    return std::make_shared<TextNode>(params); 
  }
  static std::shared_ptr<TextNode> text(std::string& text) { 
    return std::make_shared<TextNode>(text); 
  }
  static std::shared_ptr<TextNode> text(std::string& text, int size) { 
    return std::make_shared<TextNode>(text, size); 
  }
  static std::shared_ptr<TextNode> text(std::string& text, int size, std::string& font) { 
    return std::make_shared<TextNode>(text, size, font); 
  }

  std::string toString() const override;
  std::string name() const override { return "text"; }

  virtual std::vector<const class Geometry *> createGeometryList() const;

  virtual FreetypeRenderer::Params get_params() const;

  FreetypeRenderer::Params params;
};
