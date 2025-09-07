#include "core/NamespaceSpecifier.h"

#include <ostream>
#include <string>

void NamespaceSpecifier::print(std::ostream& stream, const std::string& indent) const
{
  stream << indent << this->name << ";\n";
}

std::ostream& operator<<(std::ostream& stream, const NamespaceSpecifierList& specifiers)
{
  bool first = true;
  for (const auto& spec : specifiers) {
    if (first) {
      first = false;
    } else {
      stream << ", ";
    }
    if (!spec->getName().empty()) {
      stream << spec->getName();
    }
  }
  return stream;
}
