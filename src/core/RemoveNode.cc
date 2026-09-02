#include "core/RemoveNode.h"

std::string RemoveNode::name() const
{
  return "remove";
}

std::string RemoveNode::toString() const
{
  return this->name() + "()";
}
