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

This _DOES NOT_ allow placing positional parameters _after_ named ones.  I felt
that it was too confusing.  Example:

```openscad
function fn(a,b,c,d) =
  echo(a=a, b=b, c=c, d=d)
;
x = fn("a", c="c", "d");
```

Personally, I would have expected:

```text
ECHO: a = "a", b = undef, c = "c", d = "d"
```

Instead I got:

```text
ECHO: a = "a", b = "d", c = "c", d = undef
```

So I just nixed it.

It also _DOES NOT_ perform semantic validation (type/range/meaning); callers do that
after normalization.  Might think about doing that in future if can establish a
specification that isn't crazy.

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
