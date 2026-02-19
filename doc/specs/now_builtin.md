# now builtin specification

Author: _Adrian Hawryluk_ (a.k.a. [Ma-XX-oN](https://github.com/Ma-XX-oN))

## Signature

- `now() -> number`

## Contract

- Returns a numeric microsecond timestamp from a monotonic wall-clock source.
- Return type is OpenSCAD `number` (double).
- Callers should use `now() - start` to compute elapsed time.

## Argument rules

- `now()` accepts zero arguments only.
- Any positional or named argument is invalid and produces the standard
  argument-count warning behavior.
