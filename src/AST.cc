#include "AST.h"
#include <sstream>

const Location Location::NONE(0, 0, 0, 0, nullptr);

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

void ASTNode::addAnnotations(AnnotationList *annotations)
{
	for (auto &annotation : *annotations) {
		this->annotations.insert({annotation.getName(), &annotation});
	}
}

bool ASTNode::hasAnnotations() const
{
	return !annotations.empty();
}

const Annotation * ASTNode::annotation(const std::string &name) const
{
	auto found = annotations.find(name);
	return found == annotations.end() ? nullptr : found->second;
}
