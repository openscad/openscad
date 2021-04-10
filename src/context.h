#pragma once

#include "Assignment.h"
#include "contextframe.h"

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
		ctx->session()->push_frame(ctx.get());
    }
    ~ContextHandle() {
		ctx->session()->pop_frame();
    }

	ContextHandle(const ContextHandle&) = delete;
	ContextHandle& operator=(const ContextHandle&) = delete;
	ContextHandle(ContextHandle&&) = default;
	ContextHandle& operator=(ContextHandle&&) = delete;
	
	// Valid only if ctx is on the top of the stack.
	ContextHandle& operator=(std::shared_ptr<T>&& sp)
	{
		ctx->session()->pop_frame();
		ctx = std::move(sp);
		ctx->session()->push_frame(ctx.get());
		return *this;
	}

	const T* operator->() const { return ctx.get(); }
    T* operator->() { return ctx.get(); }

    std::shared_ptr<T> ctx;
};

class Context : public ContextFrame, public std::enable_shared_from_this<Context>
{
protected:
	Context(EvaluationSession* session);
	Context(const std::shared_ptr<Context>& parent);

public:
    template<typename C, typename ... T>
    static ContextHandle<C> create(T&& ... t) {
        return ContextHandle<C>{std::shared_ptr<C>(new C(std::forward<T>(t)...))};
    }

	virtual void init() { }

	std::shared_ptr<Context> get_shared_ptr() const { return const_cast<Context*>(this)->shared_from_this(); }
	const std::shared_ptr<Context> &getParent() const { return this->parent; }

	const Value& lookup_variable(const std::string &name, bool silent = false, const Location &loc=Location::NONE) const;
	boost::optional<CallableFunction> lookup_function(const std::string &name, const Location &loc) const;
	boost::optional<InstantiableModule> lookup_module(const std::string &name, const Location &loc) const;

	double lookup_variable_with_default(const std::string &variable, const double &def) const;
	const std::string& lookup_variable_with_default(const std::string &variable, const std::string &def) const;

	void setVariables(const std::shared_ptr<EvalContext> &evalctx, const AssignmentList &parameters, const AssignmentList &optional_parameters={}, bool usermodule=false);

protected:
	const std::shared_ptr<Context> parent;

public:
#ifdef DEBUG
	virtual std::string dump(const class AbstractModule *mod, const ModuleInstantiation *inst);
#endif
};
