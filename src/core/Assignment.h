#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "AST.h"
#include "memory.h"
#include "Annotation.h"

class Assignment : public ASTNode
{
public:
  Assignment(std::string name, const Location& loc)
    : ASTNode(loc), name(name), locOfOverwrite(Location::NONE) { }
  Assignment(std::string name,
             shared_ptr<class Expression> expr = shared_ptr<class Expression>(),
             const Location& loc = Location::NONE)
    : ASTNode(loc), name(name), expr(expr), locOfOverwrite(Location::NONE){ }

  void print(std::ostream& stream, const std::string& indent) const override;
  const std::string& getName() const { return name; }
  const shared_ptr<Expression>& getExpr() const;
  const AnnotationMap& getAnnotations() const { return annotations; }
  // setExpr used by customizer ParameterObject etc.
  void setExpr(shared_ptr<Expression> e) { expr = std::move(e); }

  virtual void addAnnotations(AnnotationList *annotations);
  virtual bool hasAnnotations() const;
  virtual const Annotation *annotation(const std::string& name) const;

  const Location& locationOfOverwrite() const { return locOfOverwrite; }
  void setLocationOfOverwrite(const Location& locOfOverwrite) { this->locOfOverwrite = locOfOverwrite; }

  void gatherChilderen(std::vector<const ASTNode*>& nodes) const override;

protected:
  const std::string name;
  shared_ptr<class Expression> expr;
  AnnotationMap annotations;
  Location locOfOverwrite;
};

template <class ... Args> shared_ptr<Assignment> assignment(Args... args) {
  return make_shared<Assignment>(args ...);
}

typedef std::vector<shared_ptr<Assignment>> AssignmentList;
typedef std::unordered_map<std::string, const Expression *> AssignmentMap;

std::ostream& operator<<(std::ostream& stream, const AssignmentList& assignments);
