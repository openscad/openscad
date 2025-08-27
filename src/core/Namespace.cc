#include "core/Namespace.h"
#include "core/LocalScope.h"

void Namespace::print(std::ostream& stream, const std::string& indent) const
{
  stream << indent << "namespace " << this->name << " {\n";
  // TODO: coryrc - Every print is going to print the entire scope and not just the ones defined in this
  // location.
  body->print(stream, indent + "\t");
  stream << indent << "}\n";
}
