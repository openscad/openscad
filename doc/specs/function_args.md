# FunctionArgs Specification <!-- omit in toc -->

Author: _Adrian Hawryluk_ (a.k.a. [Ma-XX-oN](https://github.com/Ma-XX-oN))

- [Purpose](#purpose)
- [Core Types](#core-types)
- [ParamDef](#paramdef)
- [Spec Definition](#spec-definition)
- [Normalization APIs](#normalization-apis)
- [Structural Rules](#structural-rules)
- [Variadic Block Rules](#variadic-block-rules)
- [Error Routing](#error-routing)
- [Design Constraints](#design-constraints)

This document defines `FunctionArgs::Spec`, the shared helper used to normalize
function/builtin arguments into canonical form.

## Purpose

`FunctionArgs::Spec` centralizes argument-shape handling so each builtin can:

- declare its parameter shape once
- accept mixed positional/named syntax consistently
- receive canonical fixed values (and optional variadic values)
- keep consistent structural diagnostics
- disallows positional arguments after the first named argument.  This avoids
  ambiguous reads and keeps normalization deterministic.

  Example:

  ```openscad
  function fn(a,b,c,d) =
    echo(a=a, b=b, c=c, d=d)
  ;
  x = fn("a", c="c", "d");
  ```

  If positional-after-named were allowed, users may reasonably expect:

  ```text
  ECHO: a = "a", b = undef, c = "c", d = "d"
  ```

  But instead it's:

  ```text
  ECHO: a = "a", b = "d", c = "c", d = undef
  ```

  This structural filling can produce surprising results. To keep call shape
  predictable, this helper rejects positional-after-named.  This is in line with
  other languages such as Python.

- allows an optional variadic block parameter.  It can be used named, and named
  arguments can follow it.

  Example:

  ```openscad
  x = timer_run(fn=function(a,b,c,d) 1, args="a", "b", "c", "d", name="foo");
  ```

  > NOTE:
  >
  > `"a", "b", "c", "d"` are a variadic argument block, so it doesn't conflict
  > with the positional after named rule.

- does not perform semantic validation (type/range/meaning); callers can do that
  after normalization as it's currently done within the function's execution.

  Might think about doing that in future if we can establish a specification
  that isn't crazy, but I think that that would have to be a different
  specification as there would have to be ways to disambiguate calls if merged
  into this positional/named model.

  Adding type-based overload resolution to this positional/named model would
  require a separate disambiguation layer, similar in spirit to C# overload
  resolution:

  1. Determine candidate signatures.
  2. Bind named arguments by exact parameter name.
  3. Apply positional arguments in order.
  4. Enforce positional/named ordering rules.
  5. Resolve best match by deterministic conversion ranking.
  6. Fail with explicit ambiguity diagnostics when no unique match exists.

  This is likely too complex for this helper's scope and may be harder for users
  to reason about, so `FunctionArgs::Spec` remains structural-only.

## Core Types

Defined in `src/core/FunctionArgs.h`:

- `FunctionArgs::ParamDef`
- `FunctionArgs::Spec`
- `FunctionArgs::Spec::NormalizeResult`

## ParamDef

`ParamDef` declares one fixed canonical parameter:

- required: `name`
- optional: `default_value`

Constructors support concise defaults:

- `(name)` for required/no-default slots
- `(name, "string")`
- `(name, bool)`
- `(name, int)`
- `(name, double)`
- `(name, Value)` for explicit complex defaults

If a slot is omitted and has no default, normalized value is `&Value::undefined`.

## Spec Definition

`Spec(function_name, params, variadic_block_name = nullptr)`

- `function_name` is used in diagnostics
- `params` defines fixed canonical order and names
- `variadic_block_name` optionally enables one named variadic block

Example (fixed only):

```cpp
static const FunctionArgs::Spec stop_spec{
  "timer_stop",
  {
    {"timer_id"},
    {"fmt_str", "timer {n} {mmm}:{ss}.{ddd}"},
    {"iterations", 1},
    {"output", false},
    {"delete", false},
  },
};
```

Example (fixed + named variadic block):

```cpp
static const FunctionArgs::Spec run_spec{
  "timer_run",
  {{"name"}, {"fn"}},
  "args",
};
```

## Normalization APIs

- `normalize(arguments, fail) -> std::vector<const Value *>`
- `normalizeWithVariadic(arguments, fail) -> NormalizeResult`

`NormalizeResult`:

- `fixed`: fixed canonical values in `params` order
- `variadic`: collected variadic values (if enabled/used)

## Structural Rules

Applied during normalization:

- duplicate named key is an error
- positional after first named is an error
- unknown named key is an error
- unknown `$`-prefixed named keys are ignored
- duplicate assignment to same fixed parameter is an error

## Variadic Block Rules

When `variadic_block_name` is configured:

- named `<variadic_block_name> = value` appends `value` to `variadic`
- positional values immediately after that named argument are appended to `variadic`
- collection continues until the next named argument appears
- after a non-variadic named argument, normal positional-after-named error rule applies
- extra positional values beyond fixed slots go to `variadic` (instead of count error)

## Error Routing

Callers provide `ErrorFn`:

- `using ErrorFn = std::function<void(const std::string&)>;`

`Spec` reports shape/structure failures via `fail(message)`.
Callers typically log and throw from that callback.

## Design Constraints

- `Spec` is intended to be defined `static const` at call sites.
- `Spec` is declarative and reusable.
- Shape normalization is intentionally separated from semantic/type checks.
