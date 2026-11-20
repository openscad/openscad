# OpenSCAD IDE shim

This small package makes `import openscad` available to an ordinary Python
interpreter for IDE completion and lightweight script inspection. It provides
the same public signatures as OpenSCAD's embedded API, but its model objects
are placeholders: it does not generate geometry or export files. Run a model
with the OpenSCAD application to create actual geometry.

Install it into the Python environment selected by your IDE from the OpenSCAD
repository root:

```sh
python3 -m pip install -e ./python/ide-shim
```

The type stub is copied from `doc/openscad.pyi`; keep these signatures in sync
when the embedded Python API changes.
