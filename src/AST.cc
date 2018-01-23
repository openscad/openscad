#include "AST.h"
#include <sstream>

const Location Location::NONE(0, 0, 0, 0);


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
