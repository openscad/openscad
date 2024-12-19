#include "core/AST.h"
#include <ostream>
#include <memory>
#include <sstream>
#include <string>
#include "io/fileutils.h"

const Location Location::NONE(0, 0, 0, 0, std::make_shared<fs::path>(fs::path{}));

bool operator==(Location const& lhs, Location const& rhs){
  return
    lhs.firstLine() == rhs.firstLine() &&
    lhs.firstColumn() == rhs.firstColumn() &&
    lhs.lastLine() == rhs.lastLine() &&
    lhs.lastColumn() == rhs.lastColumn() &&
    lhs.filePath() == rhs.filePath();
}

bool operator!=(Location const& lhs, Location const& rhs)
{
  return !(lhs == rhs);
}

bool Location::isNone() const {
  return ((*this) == Location::NONE);
}

std::string Location::toRelativeString(const std::string& docPath) const {
  if (this->isNone()) return "location unknown";
  return "in file " + fs_uncomplete((*path), docPath).generic_string() + ", " + "line " + std::to_string(this->firstLine());
}

std::ostream& operator<<(std::ostream& stream, const ASTNode& ast)
{
  ast.print(stream, "");
  return stream;
}

std::string ASTNode::dump(const std::string& indent) const
{
  std::ostringstream stream;
  print(stream, indent);
  return stream.str();
}
