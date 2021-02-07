#include "compiler_specific.h"
#include "ModuleInstantiation.h"
#include "evalcontext.h"
#include "expression.h"
#include "exceptions.h"
#include "printutils.h"
#include <boost/filesystem.hpp>
#include "boost-utils.h"
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

void ModuleInstantiation::print(std::ostream &stream, const std::string &indent, const bool inlined) const
{
	if (!inlined) stream << indent;
	stream << modname + "(";
	for (size_t i=0; i < this->arguments.size(); ++i) {
		const auto &arg = this->arguments[i];
		if (i > 0) stream << ", ";
		if (!arg->getName().empty()) stream << arg->getName() << " = ";
		stream << *arg->getExpr();
	}
	if (scope.numElements() == 0) {
		stream << ");\n";
	} else if (scope.numElements() == 1) {
		stream << ") ";
		scope.print(stream, indent, true);
	} else {
		stream << ") {\n";
		scope.print(stream, indent + "\t", false);
		stream << indent << "}\n";
	}
}

void IfElseModuleInstantiation::print(std::ostream &stream, const std::string &indent, const bool inlined) const
{
	ModuleInstantiation::print(stream, indent, inlined);
	if (else_scope) {
		auto num_elements = else_scope->numElements();
		if (num_elements == 0) {
			stream << indent << "else;";
		}
		else {
			stream << indent << "else ";
			if (num_elements == 1) {
				else_scope->print(stream, indent, true);
			}
			else {
				stream << "{\n";
				else_scope->print(stream, indent + "\t", false);
				stream << indent << "}\n";
			}
		}
	}
}

/**
 * This is separated because PRINTB uses quite a lot of stack space
 * and the method using it evaluate()
 * is called often when recursive modules are evaluated.
 * noinline is required, as we here specifically optimize for stack usage
 * during normal operating, not runtime during error handling.
*/
static void NOINLINE print_trace(const ModuleInstantiation *mod, const std::shared_ptr<Context> ctx){
	LOG(message_group::Trace,mod->location(),ctx->documentPath(),"called by '%1$s'",mod->name());
}

AbstractNode *ModuleInstantiation::evaluate(const std::shared_ptr<Context> ctx) const
{
	ContextHandle<EvalContext> c{Context::create<EvalContext>(ctx, this->arguments, this->loc, &this->scope)};

#if 0 && DEBUG
	LOG(message_group::None,Location::NONE,"","New eval ctx:");
	c.dump(nullptr, this);
#endif
	try{
		AbstractNode *node = ctx->instantiate_module(*this, c.ctx); // Passes c as evalctx
		return node;
	}catch(EvaluationException &e){
		if(e.traceDepth>0){
			print_trace(this, ctx);
			e.traceDepth--;
		}
		throw;
	}
}

std::vector<AbstractNode*> ModuleInstantiation::instantiateChildren(const std::shared_ptr<Context> evalctx) const
{
	return this->scope.instantiateChildren(evalctx);
}

LocalScope* IfElseModuleInstantiation::makeElseScope()
{
	this->else_scope = std::make_unique<LocalScope>();
	return this->else_scope.get();
}

std::vector<AbstractNode*> IfElseModuleInstantiation::instantiateElseChildren(const std::shared_ptr<Context> evalctx) const
{
	return this->else_scope->instantiateChildren(evalctx);
}

