#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/optional.hpp>
#include "Assignment.h"
#include "function.h"
#include "memory.h"
#include "value.h"
#include "valuemap.h"

/**
 * Local handle to a all context objects. This is used to maintain the
 * dynamic scoping stack using object lifetime.
 * The Context objects can hang around for longer, e.g. in case of closures.
 */
template<typename T>
struct ContextHandle
{
    ContextHandle(std::shared_ptr<T>&& sp) : ctx(std::move(sp)) {
		ctx->init();
		ctx->push(ctx);
    }
    ~ContextHandle() {
		ctx->pop();
    }

	ContextHandle(const ContextHandle&) = delete;
	ContextHandle& operator=(const ContextHandle&) = delete;
	ContextHandle(ContextHandle&&) = default;
	ContextHandle& operator=(ContextHandle&&) = delete;
	
	// Valid only if ctx is on the top of the stack.
	ContextHandle& operator=(std::shared_ptr<T>&& sp)
	{
		ctx->pop();
		ctx = std::move(sp);
		ctx->push(ctx);
		return *this;
	}

	const T* operator->() const { return ctx.get(); }
    T* operator->() { return ctx.get(); }

    std::shared_ptr<T> ctx;
};

class EvalContext;

class Context : public std::enable_shared_from_this<Context>
{
protected:
	Context(const std::shared_ptr<Context> parent = std::shared_ptr<Context>());

public:
	typedef std::vector<std::shared_ptr<Context>> Stack;

	std::shared_ptr<Context> get_shared_ptr() { return shared_from_this(); }
	void push(std::shared_ptr<Context> ctx);
	void pop();

    template<typename C, typename ... T>
    static ContextHandle<C> create(T&& ... t) {
        return ContextHandle<C>{std::shared_ptr<C>(new C(std::forward<T>(t)...))};
    }

	virtual ~Context();
	virtual void init() { }

	const std::shared_ptr<Context> &getParent() const { return this->parent; }

	virtual boost::optional<CallableFunction> lookup_local_function(const std::string &name) const;
	boost::optional<CallableFunction> lookup_function(const std::string &name) const;
	virtual class AbstractNode *instantiate_module(const class ModuleInstantiation &inst, const std::shared_ptr<EvalContext>& evalctx) const;

	void setVariables(const std::shared_ptr<EvalContext> &evalctx, const AssignmentList &args, const AssignmentList &optargs={}, bool usermodule=false);

	void set_variable(const std::string &name, Value&& value);

	void apply_variables(const std::shared_ptr<Context> &other);
	void apply_config_variables(const std::shared_ptr<Context> &other);
	const Value& lookup_variable(const std::string &name, bool silent = false, const Location &loc=Location::NONE) const;
	double lookup_variable_with_default(const std::string &variable, const double &def, const Location &loc=Location::NONE) const;
	const std::string& lookup_variable_with_default(const std::string &variable, const std::string &def, const Location &loc=Location::NONE) const;
	Value lookup_local_config_variable(const std::string &name) const;

	bool has_local_variable(const std::string &name) const;

	void setDocumentPath(const std::string &path) { this->document_path = std::make_shared<std::string>(path); }
	const std::string &documentPath() const { return *this->document_path; }
	std::string getAbsolutePath(const std::string &filename) const;
        
public:

protected:
	const std::shared_ptr<Context> parent;
	Stack *ctx_stack;
	ValueMap variables;
	ValueMap config_variables;

	std::shared_ptr<std::string> document_path;

public:
#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
};
