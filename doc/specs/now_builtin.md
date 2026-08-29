# now builtin specification

Author: _Adrian Hawryluk_ (a.k.a. [Ma-XX-oN](https://github.com/Ma-XX-oN))

## Signature

- `now(unit="u") -> number`

## Contract

- Returns a numeric timestamp from a monotonic wall-clock source.
- Return type is OpenSCAD `number` (double).
- Callers should use `now() - start` to compute elapsed time.
- The default unit is microseconds (`"u"`).

## Argument rules

- `now()` accepts zero arguments or one optional `unit` argument.
- `unit` accepts `"u"` for microseconds, `"m"` for milliseconds, and `"s"` for seconds.
- `unit` may be supplied positionally (`now("m")`) or by name (`now(unit="m")`).
- Unknown parameter names keep the standard parameter warning behavior.
- Unsupported unit strings warn and return `undef`.
