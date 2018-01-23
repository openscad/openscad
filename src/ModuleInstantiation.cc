#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "expression.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

ModuleInstantiation::~ModuleInstantiation()
{
}

IfElseModuleInstantiation::~IfElseModuleInstantiation()
{
}

/*!
	Returns the absolute path to the given filename, unless it's empty.

	NB! This will actually search for the file, to be backwards compatible with <= 2013.01
	(see issue #217)
*/
std::string ModuleInstantiation::getAbsolutePath(const std::string &filename) const
{
	if (!filename.empty() && !fs::path(filename).is_absolute()) {
		return fs::absolute(fs::path(this->modpath) / filename).string();
	}
	else {
		return filename;
	}
}

void ModuleInstantiation::print(std::ostream &stream, const std::string &indent) const
{
	stream << indent;
	stream << modname + "(";
	for (size_t i=0; i < this->arguments.size(); i++) {
		const Assignment &arg = this->arguments[i];
		if (i > 0) stream << ", ";
		if (!arg.name.empty()) stream << arg.name << " = ";
		stream << *arg.expr;
	}
	if (scope.numElements() == 0) {
		stream << ");\n";
	} else if (scope.numElements() == 1) {	
		stream << ") ";
		scope.print(stream, "");
	} else {
		stream << ") {\n";
		scope.print(stream, indent + "\t");
		stream << indent << "}\n";
	}
}

void IfElseModuleInstantiation::print(std::ostream &stream, const std::string &indent) const
{
	ModuleInstantiation::print(stream, indent);
	stream << indent;
	if (else_scope.numElements() > 0) {
		stream << indent << "else ";
		if (else_scope.numElements() == 1) {
			else_scope.print(stream, "");
		}
		else {
			stream << "{\n";
			else_scope.print(stream, indent + "\t");
			stream << indent << "}\n";
		}
	}
}

AbstractNode *ModuleInstantiation::evaluate(const Context *ctx) const
{
	EvalContext c(ctx, this->arguments, &this->scope);

#if 0 && DEBUG
	PRINT("New eval ctx:");
	c.dump(nullptr, this);
#endif

	AbstractNode *node = ctx->instantiate_module(*this, &c); // Passes c as evalctx
	return node;
}

std::vector<AbstractNode*> ModuleInstantiation::instantiateChildren(const Context *evalctx) const
{
	return this->scope.instantiateChildren(evalctx);
}

std::vector<AbstractNode*> IfElseModuleInstantiation::instantiateElseChildren(const Context *evalctx) const
{
	return this->else_scope.instantiateChildren(evalctx);
}

