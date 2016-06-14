#pragma once

#include "AST.h"
#include "localscope.h"
#include <vector>

typedef std::vector<class ModuleInstantiation*> ModuleInstantiationList;

class ModuleInstantiation : public ASTNode
{
public:
	ModuleInstantiation(const std::string &name = "", const Location &loc = Location::NONE)
		: ASTNode(loc), tag_root(false), tag_highlight(false), tag_background(false), modname(name) { }
	virtual ~ModuleInstantiation();

	virtual std::string dump(const std::string &indent) const;
	class AbstractNode *evaluate(const class Context *ctx) const;
	std::vector<AbstractNode*> instantiateChildren(const Context *evalctx) const;

	void setPath(const std::string &path) { this->modpath = path; }
	const std::string &path() const { return this->modpath; }
	std::string getAbsolutePath(const std::string &filename) const;

	const std::string &name() const { return this->modname; }
	bool isBackground() const { return this->tag_background; }
	bool isHighlight() const { return this->tag_highlight; }
	bool isRoot() const { return this->tag_root; }

	AssignmentList arguments;
	LocalScope scope;

	bool tag_root;
	bool tag_highlight;
	bool tag_background;
protected:
	std::string modname;
	std::string modpath;
};

class IfElseModuleInstantiation : public ModuleInstantiation {
public:
	IfElseModuleInstantiation(const Location &loc) : ModuleInstantiation("if", loc) { }
	virtual ~IfElseModuleInstantiation();
	std::vector<AbstractNode*> instantiateElseChildren(const Context *evalctx) const;
	virtual std::string dump(const std::string &indent) const;

	LocalScope else_scope;
};

