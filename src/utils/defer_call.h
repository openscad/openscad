#include <tuple>
#include <type_traits>
#include <utility>

/**
 * Return a zero-argument callable that defers invoking `f` with the given args.
 *
 * Storage semantics
 * -----------------
 * - Lvalue arguments are copied (via std::decay); rvalues are moved into storage.
 * - Arrays and function types decay (e.g., `const char[N]` -> `const char*`).
 *   If you must preserve array bounds, use an array-preserving variant.
 *
 * Invocation semantics
 * --------------------
 * - On invocation, both the stored callable and stored arguments are moved
 *   into the call: `std::apply(std::move(f), std::move(tuple))`.
 * - The returned callable is single-shot.  A second invocation uses moved-from
 *   values and is generally not meaningful.
 * - Copying the returned callable *before* invoking it yields independent
 *   copies with their own unconsumed state.
 *
 * Requirements / notes
 * --------------------
 * - `F` must be a concrete callable (lambda/functor/function pointer or
 *   pointer-to-member). Overloaded names and function templates require
 *   disambiguation or a wrapper.
 * - The returned closure is copyable only if `std::decay_t<F>` and all
 *   `std::decay_t<Args>` are copyable (relevant if storing in `std::function`).
 */
template <class F, class... Args>
auto defer_call(F&& f, Args&&...args)
{
  using Params = std::tuple<std::decay_t<Args>...>;  // copy lvalues, move rvalues
  using Fn = std::decay_t<F>;                        // own a copy of the callable

  // Lambda is mutable to be able to move the callable and params out of the closure.
  return [params = Params(std::forward<Args>(args)...),
          fn = Fn(std::forward<F>(f))]() mutable -> std::invoke_result_t<Fn&&, std::decay_t<Args>&&...> {
    // Move the stored callable and the stored values into the call.
    return std::apply(std::move(fn), std::move(params));
  };
}
