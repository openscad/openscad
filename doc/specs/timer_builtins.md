# Timer Builtins Design Requirements <!-- omit in toc -->

Author: _Adrian Hawryluk_ (a.k.a. [Ma-XX-oN](https://github.com/Ma-XX-oN))

- [Scope](#scope)
- [Signatures](#signatures)
- [Argument Shape Rules](#argument-shape-rules)
- [Overload-Like Accepted Forms](#overload-like-accepted-forms)
- [Units and Return Values](#units-and-return-values)
- [Timer Types](#timer-types)
- [Type Rules](#type-rules)
- [Lifecycle and State Rules](#lifecycle-and-state-rules)
- [Echo Behavior](#echo-behavior)
- [timer\_run](#timer_run)
- [Formatting (`timer_stop` / `timer_elapsed`)](#formatting-timer_stop--timer_elapsed)
- [Error Model](#error-model)

This document defines the behavior contract for OpenSCAD `timer_*` builtins.

## Scope

These timers are for script-level ___expression evaluation___ profiling and
diagnostics. Timer storage is per evaluation/render session.  It ___does not___
profile script parsing or module rendering as those are different execution
pathways.

## Signatures

- `timer_new(name="", type="monotonic", start=false) -> number`
- `timer_start(timer_id) -> undef`
- `timer_clear(timer_id) -> undef`
- `timer_stop(timer_id, fmt_str="timer {n} {mmm}:{ss}.{dddddd}", iterations=1, output=false, delete=false) -> number | string`
- `timer_elapsed(timer_id, fmt_str="timer {n} {mmm}:{ss}.{dddddd}", iterations=1, output=false) -> number | string`
- `timer_delete(timer_id) -> undef`
- `timer_run(name, fn, args..., fmt_str="timer {n} {mmm}:{ss}.{dddddd}", iterations=1) -> any`

## Argument Shape Rules

All timer builtins use canonical argument normalization (`FunctionArgs::Spec`).
See [FunctionArgs Specification](function_args.md) for the canonical argument-shape rules, including
named/positional constraints and `timer_run` variadic-block behavior.
For `timer_run`, parameters declared after variadic `args` must be passed by name.

## Overload-Like Accepted Forms

`timer_new`:

- `timer_new()`
- `timer_new(name)`
- `timer_new(name, type)`
- `timer_new(name, type, start)`
- `timer_new(name=..., type=..., start=...)`

`timer_stop`:

- `timer_stop(timer_id)`
- `timer_stop(timer_id, fmt_str)`
- `timer_stop(timer_id, undef  )`
- `timer_stop(timer_id, fmt_str, iterations)`
- `timer_stop(timer_id, undef  , iterations)`
- `timer_stop(timer_id, fmt_str, iterations, output)`
- `timer_stop(timer_id, undef  , iterations, output)`
- `timer_stop(timer_id, fmt_str, iterations, output, delete)`
- `timer_stop(timer_id, undef  , iterations, output, delete)`
- `timer_stop(timer_id=..., fmt_str=..., iterations=..., output=..., delete=...)`

`timer_elapsed`:

- `timer_elapsed(timer_id)`
- `timer_elapsed(timer_id, fmt_str)`
- `timer_elapsed(timer_id, undef  )`
- `timer_elapsed(timer_id, fmt_str, iterations)`
- `timer_elapsed(timer_id, undef  , iterations)`
- `timer_elapsed(timer_id, fmt_str, iterations, output)`
- `timer_elapsed(timer_id, undef  , iterations, output)`
- `timer_elapsed(timer_id=..., fmt_str=..., iterations=..., output=...)`

`timer_run`:

- `timer_run(name, fn)`
- `timer_run(name, fn, args...)`
- `timer_run(name, fn, args..., fmt_str=..., iterations=...)`

## Units and Return Values

- Timer values are in microseconds (`μs`).
- Numeric values are OpenSCAD `number` (double).
- `timer_stop()` and `timer_elapsed()` return:
  - formatted `string` when `fmt_str` is a string
  - numeric microseconds when `fmt_str` is `undef`
- Returned/echoed numeric value and `{f}` use average microseconds per iteration (`elapsed/iterations`), with default `iterations=1`.

## Timer Types

- `"monotonic"` (default): monotonic wall-clock elapsed time.
- `"CPU"`: CPU elapsed time.
- Type parsing is case-insensitive.
- Any other type string is an error.

## Type Rules

- `timer_id` must be a finite number returned by `timer_new()`.
- `name` will be output when `fmt_str` contains `"{n}"`.
- `type` must be a string.
- `start`, `output`, `delete` are treated as truthy/falsy.
- `fmt_str` must be string or `undef`.
- `iterations` must be a positive integer when provided.
- `fn` in `timer_run` must be a function.

## Lifecycle and State Rules

States:

- `Stopped`
- `Running`

Rules:

- `timer_new()` creates a timer in `Stopped` state with elapsed `0`.
- If `start=true`, `timer_new()` immediately transitions to `Running`.
- `timer_start()` is valid only in `Stopped`; calling in `Running` is an error.
  Resumes accumulating from the current stored elapsed.
- `timer_stop()` is valid only in `Running`; calling in `Stopped` is an error.
  Adds elapsed since last start to stored elapsed.
- If `delete=true`, `timer_stop()` removes the timer after producing return/output.
- `timer_elapsed()` is valid in both states.
- In `Running`, `timer_elapsed()` returns stored elapsed plus current live elapsed since last start.
- In `Stopped`, `timer_elapsed()` returns stored elapsed.
- `timer_clear()` sets state to `Stopped` and elapsed to `0`.
- `timer_delete()` removes timer; deleted IDs may be reused.
- Timers are session-scoped and discarded at end of evaluation/render.

## Echo Behavior

- `timer_stop()` and `timer_elapsed()` echo only when `output=true`.
- If `fmt_str` is a string, echoed value is formatted text.
- If `fmt_str=undef`, echoed value is `timer <name_or_id> = <value> μs`.
- `timer_run()` always echoes a value using `fmt_str` (or `timer <name_or_id> = <value> μs` when `fmt_str=undef`).

Name fallback (`{n}` token and numeric echo label):

- use timer name from `timer_new(name, ...)` / `timer_run(name, ...)`
- if empty, use timer id text

## timer_run

`timer_run(name, fn, args..., fmt_str="timer {n} {mmm}:{ss}.{dddddd}", iterations=1)` behavior:

1. Create a monotonic timer with `name`.
2. Start timer.
3. Invoke `fn(args...)`.
4. Stop timer, divide elapsed microseconds by `iterations`, and echo using `fmt_str`.
5. Delete timer.
6. Return the value produced by `fn(...)`.

Errors:

- missing required `fn` (by shape)
- non-function `fn`
- invalid `fmt_str` type (must be string or `undef`)
- invalid `iterations` (must be positive integer)

## Formatting (`timer_stop` / `timer_elapsed`)

Default format:

- `"timer {n} {mmm}:{ss}.{dddddd}"`

Supported brace tokens:

- `{n}` timer name text
- `{f}` elapsed microseconds formatted using OpenSCAD's standard number-to-string
  conversion (trailing zeros stripped; e.g. `123456`, not `123456.0`)
- `{i}` iterations value
- `{h}` hours, 1+ digits
- `{hh}` hours, 2-digit zero-padded (no rollover)
- `{m}` minute within hour (`0..59`)
- `{mm}` minute within hour, 2-digit zero-padded
- `{mmm}` total minutes (no 60 rollover)
- `{s}` second within minute (`0..59`)
- `{ss}` second within minute, 2-digit zero-padded
- `{sss}` total seconds (no 60 rollover)
- `{d}`, `{dd}`, `{ddd}`, ... fractional second digits
- number of `d` characters sets decimal places
- precision source is microseconds (up to 6 meaningful digits)
- digits beyond 6 are right-padded with `0`

Escaping:

- `{{` emits literal `{`
- `}}` emits literal `}`

Validation:

- unknown token is an error
- unmatched `{` or `}` is an error

## Error Model

Misuse is a hard runtime error.  Common categories:

- wrong argument count
- wrong argument type
- invalid timer id
- invalid state transition
- invalid timer type
- invalid format token/braces
