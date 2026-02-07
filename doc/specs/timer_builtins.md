# Timer Builtins Requirements

This document defines the behavior contract for OpenSCAD `timer_*` builtins.

## Scope

These timers are intended for script-level profiling and timing diagnostics.
Timer storage is per evaluation/render session.

## Functions

- `timer_run    (name="", fn, ...) -> <any>`
- `timer_new    (name="", type="monotonic", start=false) -> number`
- `timer_start  (timer_id) -> undef`
- `timer_clear  (timer_id) -> undef`
- `timer_stop   (timer_id, fmt_str = "timer {n} {mmm}:{ss}.{ddd}", output=false, delete=false) -> number | string`
- `timer_elapsed(timer_id, fmt_str = "timer {n} {mmm}:{ss}.{ddd}", output=false) -> number | string`
- `timer_delete (timer_id) -> undef`

## Overloads

`timer_run`
- `timer_run(fn, ...) -> <any>`
- `timer_run(name, fn, ...) -> <any>`

`timer_new`
- `timer_new() -> number`
- `timer_new(name) -> number`
- `timer_new(name, type) -> number`
- `timer_new(name, start) -> number`
- `timer_new(name, type, start) -> number`
- `timer_new(start) -> number`
- `timer_new(name=..., type=..., start=...) -> number`

`timer_start`
- `timer_start(timer_id) -> undef`

`timer_clear`
- `timer_clear(timer_id) -> undef`

`timer_stop`
- `timer_stop(timer_id) -> string`
- `timer_stop(timer_id, fmt_str) -> string`
- `timer_stop(timer_id, output) -> string`
- `timer_stop(timer_id, fmt_str, output) -> string`
- `timer_stop(timer_id, fmt_str, output, delete) -> string`
- `timer_stop(timer_id, output, delete) -> string`
- `timer_stop(timer_id, undef) -> number`
- `timer_stop(timer_id, undef, output) -> number`
- `timer_stop(timer_id, undef, output, delete) -> number`
- `timer_stop(timer_id, fmt_str=..., output=..., delete=...) -> number | string`

`timer_elapsed`
- `timer_elapsed(timer_id) -> string`
- `timer_elapsed(timer_id, fmt_str) -> string`
- `timer_elapsed(timer_id, output) -> string`
- `timer_elapsed(timer_id, fmt_str, output) -> string`
- `timer_elapsed(timer_id, undef) -> number`
- `timer_elapsed(timer_id, undef, output) -> number`
- `timer_elapsed(timer_id, fmt_str=..., output=...) -> number | string`

`timer_delete`
- `timer_delete(timer_id) -> undef`

## Units and Numeric Representation

- Timer values are expressed in microseconds (`μs`).
- Values are returned as OpenSCAD `number` (double).
- `timer_stop()` and `timer_elapsed()` return either:
  - formatted `string` (default, or when `fmt_str` is a string), or
  - elapsed `μs` as `number` (when `fmt_str` is `undef`).

## Timer Types

- `"monotonic"` (default): elapsed wall-clock style monotonic time.
- `"CPU"`: elapsed CPU clock time.

Type parsing:

- Case-insensitive for accepted literals.
- Any other type string is an error.

`timer_new()` argument forms:

- positional:
  - `timer_new()`
  - `timer_new(name)`
  - `timer_new(name, type)`
  - `timer_new(name, type, start)`
  - `timer_new(name, start)` (`start` is bool)
  - `timer_new(start)` (`start` is bool)
- named:
  - `name=...`, `type=...`, `start=...` (unknown names are errors)

## Lifecycle and State Rules

Timer states:

- `Stopped`
- `Running`

Rules:

- `timer_new()` creates a timer in `Stopped` state with elapsed value `0`.
- if `start=true`, `timer_new()` immediately transitions to `Running`.
- `timer_start()` is valid only in `Stopped`; calling while `Running` is an error.
- `timer_stop()` is valid only in `Running`; calling while `Stopped` is an error.
- if `delete=true`, `timer_stop()` removes the timer after producing the return value/output.
- `timer_elapsed()` is valid in both `Running` and `Stopped`.
  - In `Running`, returns current elapsed from start.
  - In `Stopped`, returns stored elapsed.
- `timer_clear()` sets state to `Stopped` and elapsed to `0`.
- `timer_delete()` removes timer; deleted IDs may be reused.
- All timers are session-scoped and discarded at end of evaluation/render.

## ID Handling

- Timer IDs are numeric values.
- Unknown ID in any timer operation is an error.

## Echo Behavior

- `output` arguments are truthy/falsy.
- For `timer_stop()` and `timer_elapsed()`, if `output` is truthy:
  - default behavior (`fmt_str` omitted, or `fmt_str` is a string): output formatted string.
  - exception (`fmt_str = undef`): output `timer <name_or_id> = <value> μs`.
- Name fallback (`{n}` token and numeric-echo label):
  - use timer `name` from `timer_new(name, ...)`
  - if empty, use timer ID text.

## timer_run

`timer_run(name="", fn, ...)` behavior:

1. Create a monotonic timer with provided `name`.
2. Start timer.
3. Invoke `fn(...)` with remaining arguments.
4. Stop timer and echo result.
5. Delete timer.
6. Return the value produced by `fn(...)`.

Errors:

- Missing function argument.
- `fn` is not a function value.

## timer_stop / timer_elapsed formatting

`timer_stop(timer_id, fmt_str, output, delete)` and `timer_elapsed(timer_id, fmt_str, output)` format timer output.

Argument rules:

- `timer_id` must reference an existing timer.
- `fmt_str` is optional and may be:
  - `string` to request formatted `string` return
  - `undef` to request numeric (`μs`) return
- `output` is optional truthy/falsy.
- `delete` (timer_stop only) is optional truthy/falsy.
- `output` may also be supplied as argument 2 when `fmt_str` is omitted.
- `delete` may be supplied positionally as the final argument (after `output` if present), or as `delete=...`.
- If omitted, `fmt_str` defaults to string formatting.

Default format:

- `"timer {n} {mmm}:{ss}.{ddd}"`

### Format Tokens

Supported brace tokens:

- `{n}` timer name text
- `{f}` elapsed microseconds as numeric string
- `{h}` hours, 1+ digits
- `{hh}` hours, 2-digit zero-padded
- `{m}` minute within hour (`0..59`)
- `{mm}` minute within hour, 2-digit zero-padded
- `{mmm}` total minutes (no 60 rollover)
- `{s}` second within minute (`0..59`)
- `{ss}` second within minute, 2-digit zero-padded
- `{sss}` total seconds (no 60 rollover)
- `{d}`, `{dd}`, `{ddd}`, ... fractional second digits
  - Number of `d` characters = number of decimal places.
  - Precision source is microseconds (up to 6 meaningful digits).
  - Digits beyond 6 are right-padded with `0`.

Escaping:

- `{{` emits literal `{`
- `}}` emits literal `}`

Validation:

- Unknown token is an error.
- Unmatched `{` or unmatched `}` is an error.

## Error Model

- Misuse is a hard runtime error.
- Typical error categories:
  - wrong argument count
  - wrong argument type
  - invalid timer ID
  - invalid state transition
  - invalid timer type
  - invalid format token/braces
