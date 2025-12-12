#include "core/AST.h"
#include <filesystem>
#include <ostream>
#include <memory>
#include <sstream>
#include <string>
#include "io/fileutils.h"

const Location Location::NONE(0, 0, 0, 0, std::make_shared<fs::path>(fs::path{}));

bool operator==(Location const& lhs, Location const& rhs)
{
  return lhs.firstLine() == rhs.firstLine() && lhs.firstColumn() == rhs.firstColumn() &&
         lhs.lastLine() == rhs.lastLine() && lhs.lastColumn() == rhs.lastColumn() &&
         lhs.filePath() == rhs.filePath();
}

bool operator!=(Location const& lhs, Location const& rhs) { return !(lhs == rhs); }

bool Location::isNone() const { return ((*this) == Location::NONE); }

std::string Location::toRelativeString(const std::string& docPath) const
{
  if (this->isNone()) return "location unknown";
  return "in file " + fs_uncomplete((*path), docPath).generic_string() + ", " + "line " +
         std::to_string(this->firstLine());
}

std::ostream& operator<<(std::ostream& stream, const ASTNode& ast)
{
  ast.print(stream, "");
  return stream;
}

std::string ASTNode::dump(const std::string& indent) const
{
  std::ostringstream stream;
  // All constants output in the AST dump are shown with full precision (17
  // significant digits).  This precision is sufficient to losslessly represent
  // any value stored in the IEEE 754 64-bit floating point format used by
  // OpenSCAD to store the numeric values of numbers stored in Value objects.
  stream << std::setprecision(17);
  print(stream, indent);
  return stream.str();
}
