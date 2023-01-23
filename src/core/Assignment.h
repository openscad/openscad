#pragma once

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "AST.h"
#include "memory.h"
#include "Annotation.h"

class Assignment : public ASTNode
{
public:
  Assignment(std::string name, const Location& loc)
    : ASTNode(loc), name(std::move(name)), locOfOverwrite(Location::NONE) { }
  Assignment(std::string name,
             shared_ptr<class Expression> expr = shared_ptr<class Expression>(),
             const Location& loc = Location::NONE)
    : ASTNode(loc), name(std::move(name)), expr(std::move(expr)), locOfOverwrite(Location::NONE){ }

  void print(scad::ostringstream& stream, const std::string& indent) const override;
  const std::string& getName() const { return name; }
  const shared_ptr<Expression>& getExpr() const { return expr; }
  const AnnotationMap& getAnnotations() const { return annotations; }
  // setExpr used by customizer ParameterObject etc.
  void setExpr(shared_ptr<Expression> e) { expr = std::move(e); }

  virtual void addAnnotations(AnnotationList *annotations);
  virtual bool hasAnnotations() const;
  virtual const Annotation *annotation(const std::string& name) const;

  const Location& locationOfOverwrite() const { return locOfOverwrite; }
  void setLocationOfOverwrite(const Location& locOfOverwrite) { this->locOfOverwrite = locOfOverwrite; }

protected:
  const std::string name;
  shared_ptr<class Expression> expr;
  AnnotationMap annotations;
  Location locOfOverwrite;
};

template <class ... Args> shared_ptr<Assignment> assignment(Args... args) {
  return make_shared<Assignment>(args ...);
}

using AssignmentList = std::vector<shared_ptr<Assignment>>;
using AssignmentMap = std::unordered_map<std::string, const Expression *>;

scad::ostringstream& operator<<(scad::ostringstream& stream, const AssignmentList& assignments);
