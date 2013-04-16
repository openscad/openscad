#ifndef FILECONTEXT_H_
#define FILECONTEXT_H_

#include "context.h"
#include <boost/unordered_map.hpp>

/*!
	This holds the context for a Module definition; keeps track of
	global variables, submodules and functions defined inside a module.

	NB! every .scad file defines an implicit unnamed module holding the contents of the file.
*/
class ModuleContext : public Context
{
public:
	ModuleContext(const class Module *module = NULL, const Context *parent = NULL, const EvalContext *evalctx = NULL);
	virtual ~ModuleContext();

	void setModule(const Module &module, const EvalContext *evalctx = NULL);
	void registerBuiltin();

	virtual Value evaluate_function(const std::string &name, const EvalContext *evalctx) const;
	virtual AbstractNode *evaluate_module(const ModuleInstantiation &inst, const EvalContext *evalctx) const;

	const boost::unordered_map<std::string, class AbstractFunction*> *functions_p;
	const boost::unordered_map<std::string, class AbstractModule*> *modules_p;
	typedef boost::unordered_map<std::string, class Module*> ModuleContainer;
	const ModuleContainer *usedlibs_p;

  // FIXME: Points to the eval context for the call to this module. Not sure where it belongs
	const class EvalContext *evalctx;

#ifdef DEBUG
	virtual void dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif

	mutable boost::unordered_map<std::string, int> recursioncount;
};

#endif
