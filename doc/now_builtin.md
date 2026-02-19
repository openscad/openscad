# now() Builtin <!-- omit in toc -->

Author: _Adrian Hawryluk_ (a.k.a. [Ma-XX-oN](https://github.com/Ma-XX-oN))

- [Signature](#signature)
- [What It Measures](#what-it-measures)
- [Quick Start](#quick-start)
- [Evaluation Phases and now()](#evaluation-phases-and-now)
- [Common Patterns](#common-patterns)
- [Common Mistakes](#common-mistakes)

## Signature

```text
now() -> number
```

Returns the current monotonic wall-clock reading in microseconds.  The
value is meaningful only when subtracted from another `now()` call within
the same process — absolute values are not stable across runs.

`now()` takes no arguments.

## What It Measures

`now()` reads a monotonic wall-clock.  It is useful for benchmarking the
cost of OpenSCAD expressions, function calls, and — when placed in a
module argument expression — elapsed time across module instantiations
(see [Evaluation Phases and now()](#evaluation-phases-and-now)).  It does
_not_ profile script parsing, which happens before evaluation begins,
nor the backend render pass (CSG evaluation, mesh generation), which
happens after.

## Quick Start

```openscad
start = now();
result = expensive_computation();
elapsed_us = now() - start;
echo(str("elapsed: ", elapsed_us, " μs"));
```

## Evaluation Phases and now()

Understanding OpenSCAD's two-phase evaluation model is essential for using
`now()` correctly.

Inside any scope (file-level or module body), OpenSCAD processes statements
in two passes:

1. **Phase 1 — Assignments:**  All assignments (`x = ...;`) are evaluated
   in source order.  Function calls in expressions — including `now()` —
   execute during this phase.
2. **Phase 2 — Module instantiations:**  All module calls (`cube(...)`,
   `for (...)`, `echo(...)`, etc.) are executed in source order, _after_
   all assignments have completed.

This means consecutive assignments calling `now()` run back-to-back in
Phase 1, so subtracting them gives a precise measurement of the
expressions evaluated between them:

```openscad
s = now();              // Phase 1
result = expensive();   // Phase 1
elapsed = now() - s;    // Phase 1 — measures expensive()
```

However, module instantiations (`for`, `echo`, user modules) run in
Phase 2.  A `now()` call in an assignment cannot bracket a module
instantiation, because both assignments fire before any module runs.

Module _arguments_ are expressions, and they are evaluated when the module
is instantiated (Phase 2).  So `now()` inside an `echo(...)` argument
executes in Phase 2, after all assignments and any preceding module
instantiations:

```openscad
start = now();                  // Phase 1
some_module();                  // Phase 2 — runs first
echo(now() - start);            // Phase 2 — now() evaluated here
```

The `echo` reports the wall-clock time from Phase 1 (when `start` was
set) through `some_module()`'s instantiation.  This includes Phase 1→2
transition overhead, but for most purposes the measurement is useful.

The same two-phase rule applies inside nested scopes.  A module body gets
its own Phase 1 + Phase 2 cycle when the module is instantiated:

```openscad
module timed_work() {
  s = now();            // inner Phase 1
  result = work();      // inner Phase 1
  elapsed = now() - s;  // inner Phase 1 — measures work()
  echo(elapsed);        // inner Phase 2
}
```

## Common Patterns

**Average cost per call over N iterations:**

A bare `for` loop cannot interleave with `now()` calls in assignments at
the same scope (see Common Mistakes).  Use a recursive helper instead:

```openscad
function _loop(fn, i, n) = i >= n ? undef : let(_ = fn(i)) _loop(fn, i+1, n);

start = now();
_ = _loop(function(i) my_fn(i), 0, 1000);
avg_us = (now() - start) / 1000;
echo(str("avg per call: ", avg_us, " μs"));
```

**Time two alternatives and compare:**

```openscad
s1 = now();
a = method_a(data);
e1 = now() - s1;

s2 = now();
b = method_b(data);
e2 = now() - s2;

echo(str("a=", e1, " μs  b=", e2, " μs  ratio=", e1/e2));
```

**Timing a module invocation:**

Module arguments are evaluated at module-instantiation time (Phase 2), so
`now()` inside an `echo` captures the time after preceding modules run:

```openscad
start = now();
some_module();
echo(str("elapsed to this point: ", now() - start, " μs"));
```

Note: this measures wall-clock time from the end of the assignment phase
through `some_module()`'s instantiation — it includes any Phase 1→Phase 2
transition overhead.

## Common Mistakes

- **Timing a `for` loop with sandwiched `now()` calls** — In any scope,
  OpenSCAD evaluates all assignments first (Phase 1), then all module
  instantiations (Phase 2).  A bare `for` is a module instantiation.
  Two `now()` calls in assignments at the same scope both fire in Phase 1,
  before the loop runs — measuring nothing:

  ```openscad
  start = now();                       // assignment — Phase 1
  for (i = [0:999]) { x = sum(lst); } // module inst — Phase 2
  elapsed = now() - start;             // assignment — Phase 1 (!)
  ```

  You _can_ place the second `now()` inside an `echo` (also Phase 2),
  which runs after the loop, but the measurement includes Phase 1→Phase 2
  overhead and any earlier module instantiations.  For precise
  expression-only timing, use a recursive function instead (see Common
  Patterns).

- **Expecting render times from assignments** — Module instantiation
  (Phase 2) builds the CSG tree but does not perform the final render
  (mesh generation, boolean evaluation).  A `now()` call in an
  assignment (Phase 1) cannot even bracket instantiation time.  To
  measure elapsed time across instantiations, place `now()` in a module
  argument expression so it executes in Phase 2 (see
  [Evaluation Phases and now()](#evaluation-phases-and-now)).
