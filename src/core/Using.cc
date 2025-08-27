#include "core/Using.h"

void Using::print(std::ostream& stream, const std::string& indent) const
{
  stream << indent << "using " << this->name << ";\n";
}
