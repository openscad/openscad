#include "AST.h"
#include <sstream>
#include "boost-utils.h"

const Location Location::NONE(0, 0, 0, 0, nullptr, nullptr);

bool operator==(Location const& lhs, Location const& rhs){
	return
		lhs.firstLine()   == rhs.firstLine() &&
		lhs.firstColumn() == rhs.firstColumn() &&
		lhs.lastLine()    == rhs.lastLine() &&
		lhs.lastColumn()  == rhs.lastColumn() &&
		lhs.filePath()    == rhs.filePath();
}

bool operator != (Location const& lhs, Location const& rhs)
{
  return ! (lhs==rhs);
}

bool Location::isNone() const{
	return ((*this)==Location::NONE);
}

std::string Location::toString() const{
	if(this->isNone()) return "location unkown";
	if(mainfile && (*path)!=(*mainfile)){
		return "in file "+boostfs_uncomplete((*path), (*mainfile).parent_path()).generic_string()+ ", "+"line " + std::to_string(this->firstLine());
	}
	return "line " + std::to_string(this->firstLine());
}

std::ostream &operator<<(std::ostream &stream, const ASTNode &ast)
{
	ast.print(stream, "");
	return stream;
}

std::string ASTNode::dump(const std::string &indent) const
{
	std::stringstream stream;
	print(stream, indent);
	return stream.str();
}
