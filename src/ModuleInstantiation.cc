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

std::string ModuleInstantiation::dump(const std::string &indent) const
{
	std::stringstream dump;
	dump << indent;
	dump << modname + "(";
	for (size_t i = 0; i < this->arguments.size(); i++) {
		const Assignment &arg = this->arguments[i];
		if (i > 0) dump << ", ";
		if (!arg.name.empty()) dump << arg.name << " = ";
		dump << *arg.expr;
	}
	if (scope.numElements() == 0) {
		dump << ");\n";
	}
	else if (scope.numElements() == 1) {
		dump << ") ";
		dump << scope.dump("");
	}
	else {
		dump << ") {\n";
		dump << scope.dump(indent + "\t");
		dump << indent << "}\n";
	}
	return dump.str();
}

std::string IfElseModuleInstantiation::dump(const std::string &indent) const
{
	std::stringstream dump;
	dump << ModuleInstantiation::dump(indent);
	dump << indent;
	if (else_scope.numElements() > 0) {
		dump << indent << "else ";
		if (else_scope.numElements() == 1) {
			dump << else_scope.dump("");
		}
		else {
			dump << "{\n";
			dump << else_scope.dump(indent + "\t");
			dump << indent << "}\n";
		}
	}
	return dump.str();
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

std::vector<AbstractNode *> ModuleInstantiation::instantiateChildren(const Context *evalctx) const
{
	return this->scope.instantiateChildren(evalctx);
}

std::vector<AbstractNode *> IfElseModuleInstantiation::instantiateElseChildren(const Context *evalctx) const
{
	return this->else_scope.instantiateChildren(evalctx);
}

