# Sharing and using PythonSCAD libraries

Reusable geometry helpers belong in ordinary **Python packages**: versioned,
multi-file, installable with `pip`, and shareable on [PyPI](https://pypi.org/).
That is the supported way to publish a library and to depend on someone else's
work in your designs.

Prefer packages over the deprecated [`nimport()`](../reference/io.md#nimport)
URL download. See that page for why `nimport` is being phased out; this section
documents the replacement workflow.

## What you get with packages

- **Multiple files** — split models, helpers, and tests across a package layout.
- **Versions** — pin `mylib==1.2.3` so renders stay reproducible.
- **Dependencies** — declare `pythonscad` and other packages in metadata.
- **Distribution** — publish once; others install with a single `pip install`.

Distribution name and import name may differ. Example: install
[`pythonscad-pincutout`](https://pypi.org/project/pythonscad-pincutout/) with
`pip install pythonscad-pincutout`, then `import pincutout` in your script.

## Guides in this section

| Guide | Audience |
|-------|----------|
| [Creating and publishing a library](creating.md) | Authors packaging reusable geometry |
| [Using libraries in your designs](using.md) | Consumers installing packages (GUI or headless) |

## Worked examples

| Role | Project |
|------|---------|
| Full-featured library | [pythonscad-pincutout](https://github.com/nomike/pythonscad-pincutout) on GitHub / [PyPI](https://pypi.org/project/pythonscad-pincutout/) |
| Minimal design that uses it | [unpolitischer-verein-button](https://github.com/nomike/unpolitischer-verein-button) |

The teaching examples in [Creating](creating.md) stay smaller than
`pythonscad-pincutout`; use that repository when you want CI, Trusted Publishing,
and a richer API as a reference.
