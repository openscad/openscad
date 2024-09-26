#pragma once

#include <ostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class Annotation
{
public:
  Annotation(std::string name, std::shared_ptr<class Expression> expr);

  void print(std::ostream& stream, const std::string& indent) const;
  [[nodiscard]] const std::string& getName() const { return name; }
  [[nodiscard]] const std::shared_ptr<Expression>& getExpr() const { return expr; }

private:
  std::string name;
  std::shared_ptr<Expression> expr;
};

using AnnotationList = std::vector<Annotation>;
using AnnotationMap = std::unordered_map<std::string, Annotation *>;
