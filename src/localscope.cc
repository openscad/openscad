#include "localscope.h"
#include "modcontext.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "UserModule.h"
#include "expression.h"
#include "function.h"
#include "annotation.h"
#include "UserModule.h"

LocalScope::LocalScope()
{
}

LocalScope::~LocalScope()
{
	for (auto &v : children) delete v;
	for (auto &f : functions) delete f.second;
	for (auto &m : modules) delete m.second;
}

void LocalScope::addChild(ModuleInstantiation *modinst) 
{
	assert(modinst);
	this->children.push_back(modinst);
}

void LocalScope::addModule(const std::string &name, class UserModule *module)
{
	assert(module);
	this->modules[name] = module;
	this->astModules.push_back({name, module});
}

void LocalScope::addFunction(class UserFunction *func)
{
	assert(func);
	this->functions[func->name] = func;
	this->astFunctions.push_back({func->name, func});
}

void LocalScope::addAssignment(const Assignment &ass)
{
	this->assignments.push_back(ass);
}

void LocalScope::print(std::ostream &stream, const std::string &indent, const bool inlined) const
{
	for (const auto &f : this->astFunctions) {
		f.second->print(stream, indent);
	}
	for (const auto &m : this->astModules) {
		m.second->print(stream, indent);
	}
	// FIXME: Take note of this behavior change: All assignments printed,
	// not just the last one for each variable
	for (const auto &ass : this->assignments) {
		ass.print(stream, indent);
	}
	for (const auto &inst : this->children) {
		inst->print(stream, indent, inlined);
	}
}

std::vector<AbstractNode*> LocalScope::instantiateChildren(const std::shared_ptr<Context> evalctx) const
{
	std::vector<AbstractNode*> childnodes;
	for(const auto &modinst : this->children) {
		AbstractNode *node = modinst->evaluate(evalctx);
		if (node) childnodes.push_back(node);
	}

	return childnodes;
}

/*!
	When instantiating a module which can take a scope as parameter (i.e. non-leaf nodes),
	use this method to apply the local scope definitions to the evaluation context.
	This will enable variables defined in local blocks.
	NB! for loops are special as the local block may depend on variables evaluated by the
	for loop parameters. The for loop code will handle this specially.
*/
void LocalScope::apply(const std::shared_ptr<Context> ctx) const
{
	// FIXME: Filter assignments before doing this
	for(const auto &assignment : this->assignments) {
		ctx->set_variable(assignment.name, assignment.expr->evaluate(ctx));
	}
}

/*!
	This implements OpenSCAD's variable re-assignment rules:

	If two variables with the same name are assigned the same value in
	the same scope, the second assignment will "win", but will be
	evaluated in the lexical location of the initial assignment.

	This also implements warnings to help people resolve unwanted reassignments.
 */
void LocalScope::resolveAssignments()
{
	for (auto it = this->assignments.begin(); it != this->assignments.end(); it++) {
		auto &currAssignment = *it;
		for (auto curr_it = this->assignments.begin(); curr_it != it; curr_it++) {
			auto &assignment = *curr_it;
			if (assignment.name == currAssignment.name) {
				// FIXME: Re-enable warnings
				/*
				auto mainFile = mainFilePath.string();
				auto prevFile = assignment.location().fileName();
				auto currFile = currAssignment.location().fileName();

				const auto uncPathCurr = boostfs_uncomplete(currFile, mainFilePath.parent_path());
				const auto uncPathPrev = boostfs_uncomplete(prevFile, mainFilePath.parent_path());
				if (assignment.isOverride) {
					//assignments via commandline
				} else if (prevFile == mainFile && currFile == mainFile) {
					//both assignments in the mainFile
					PRINTB("WARNING: %s was assigned on line %i but was overwritten on line %i",
								 assignment.name %
								 assignment.location().firstLine() %
								 loc.firstLine());
				} else if (uncPathCurr == uncPathPrev) {
					//assignment overwritten within the same file
					//the line number being equal happens, when a file is included multiple times
					if (assignment.location().firstLine() != loc.firstLine()) {
						PRINTB("WARNING: %s was assigned on line %i of %s but was overwritten on line %i",
									 assignment.name %
									 assignment.location().firstLine() %
									 uncPathPrev %
									 loc.firstLine());
					}
				} else if (prevFile == mainFile && currFile != mainFile) {
					//assignment from the mainFile overwritten by an include
					PRINTB("WARNING: %s was assigned on line %i of %s but was overwritten on line %i of %s",
								 assignment.name %
								 assignment.location().firstLine() %
								 uncPathPrev %
								 loc.firstLine() %
								 uncPathCurr);
				}
				*/
 				assignment.expr = currAssignment.expr;
				assignment.setLocation(currAssignment.location());
				currAssignment.isDisabled = true;
				break;
			}
		}
	}
}
