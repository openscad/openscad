# Browser / WASM API

PythonSCAD ships a WebAssembly build that runs the full geometry kernel and an
embedded CPython interpreter entirely in the browser — no installation, no
server round-trips. If you just want to *use* it, open the
[Playground](https://www.pythonscad.org/playground/). This page documents the
small JavaScript API you can use to embed PythonSCAD in your own pages.

There are two ways to drive the module: a tiny **embedding API** (best for
REPL/notebook-style use) and the **CLI path** (best for one-shot file
conversion, e.g. rendering an STL).

## Loading the module

The web build is an ES module that exports a factory. Provide `print` /
`printErr` callbacks to capture Python's stdout/stderr:

```javascript
import factory from './pythonscad.js';

const mod = await factory({
  print:    (s) => console.log(s),
  printErr: (s) => console.warn(s),
});
```

`mod.cwrap(...)` then gives typed wrappers around the exported C functions.

## Embedding API

Three C functions are exported for embedding (defined in
`src/python/pyopenscad.cc`). Wrap them with `cwrap`:

```javascript
const initPython     = mod.cwrap('EmsInitPython',     null,     []);
const evaluatePython = mod.cwrap('EmsEvaluatePython', 'string', ['string', 'number']);
const finishPython   = mod.cwrap('EmsFinishPython',   null,     []);
```

### `EmsInitPython()`

```c
void EmsInitPython(void)
```

One-time bootstrap of the CPython runtime and PythonSCAD library path. Selects
the Manifold geometry backend. Call it exactly **once** per module instance,
before any evaluation.

Do **not** call it again to "reset" a session: after a successful first init a
second call takes the re-init path in `initPython`, which removes user-added
globals from `__main__` and reloads the overlay modules. It therefore does not
preserve interpreter state. To start fresh, create a new module instance
instead.

### `EmsEvaluatePython(code, dryRun)`

```c
const char *EmsEvaluatePython(const char *code, bool dry_run)
```

Runs a Python source string in the persistent `__main__` namespace, so state
(imports, variables, function definitions) carries over between calls — exactly
what a notebook needs.

- **Return value** — an empty string on success, or a formatted Python
  traceback on error. It does **not** contain stdout/stderr; those go to the
  `print` / `printErr` callbacks you passed to the factory.
- **`dryRun`** — pass `0`/`false` for a normal run, or non-zero/`true` to skip
  geometry generation (e.g. for syntax checks).
- **Lifetime** — the returned pointer references a `static std::string` that is
  overwritten by the next call; copy it out before calling again.
- Unlike the CLI path, this does **not** require `--trust-python`.

```javascript
initPython();
const err = evaluatePython('from pythonscad import *\nshow(cube(10))', false);
if (err) console.error(err);
```

### `EmsFinishPython()`

```c
void EmsFinishPython(void)
```

Finalizes a session by unioning the objects passed to `show()` into the result
node for the downstream render/export pipeline, and clears the dry-run flag.

Notebook-style integrations that read geometry back from Python directly (see
below) do not need this; it is mainly relevant when combining the embedding API
with the CLI export path.

### Getting geometry out

The embedding API runs code but does not, by itself, hand mesh data back to
JavaScript. The [Playground](https://www.pythonscad.org/playground/) does this
by overriding `show()` in a small Python prelude so it `print()`s the mesh
(vertices/faces/colors) as JSON between sentinel markers, which the page parses
out of the `print` stream. This keeps the JS surface minimal while supporting
arbitrary geometry. For full STL/3MF/OBJ export, use the CLI path instead.

## CLI path (`callMain`)

For one-shot conversion (script in, file out) use the OpenSCAD command-line
entry point via Emscripten's virtual filesystem:

```javascript
mod.FS.writeFile('/input.py', 'from pythonscad import *\nshow(cube(10))');

let exitCode = 0;
try {
  mod.callMain(['-o', '/output.stl', '--trust-python', '/input.py']);
} catch (e) {
  exitCode = (e && typeof e.status === 'number') ? e.status : 1;
}

const stl = mod.FS.readFile('/output.stl');   // Uint8Array
```

Notes:

- `--trust-python` is **required** for the CLI path; without it the evaluator
  refuses to run.
- `INVOKE_RUN=0` is set, so `main()` does not run on load — call `callMain`
  explicitly.
- The module is not re-entrant for the CLI path: create a fresh instance per
  render if you need repeated or concurrent use.

## See also

- [Building for WebAssembly](../building-wasm.md) — how the WASM bundle is built.
- [Playground](https://www.pythonscad.org/playground/) — the live in-browser notebook.
