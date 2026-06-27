# WebAssembly Build Guide

PythonSCAD can be compiled to WebAssembly (WASM) using Emscripten, allowing it
to run entirely in a web browser with full Python scripting support.

## Architecture

### Why monolithic Emscripten (not Pyodide)

PythonSCAD registers `_openscad` as a CPython built-in extension via
`PyImport_AppendInittab("_openscad", &PyInit__openscad)`. Built-in modules
cannot be dynamically loaded — they must be baked into the interpreter at
link time. This makes a single monolithic binary (CPython + geometry engine,
linked together by Emscripten) the natural fit; Pyodide would require
restructuring the entire geometry engine as a Pyodide wheel.

CPython 3.14 has official `wasm32-emscripten` tier-3 support and cross-compiles
cleanly. We build only `libpython3.14.a` (not the standalone `python.js`)
to avoid extension modules (`_decimal`, `_sha2`, etc.) that need C libraries
absent from the WASM sysroot.

### Build variants

| Variant | Filesystem                   | Use case                          |
| ------- | ---------------------------- | --------------------------------- |
| `node`  | NODERAWFS (real FS)          | Smoke testing, headless rendering |
| `web`   | MEMFS + preloaded `.data`    | Browser distribution              |

## Prerequisites

### Docker base image

The WASM build uses two Docker images, both on **Emscripten 6.0** (required for
CPython 3.14 browser support):

1. **`pythonscad-wasm-sysroot:local`** — cross-compiles third-party libraries
   (Eigen, Boost, CGAL, …) using the
   [openscad-wasm](https://github.com/openscad/openscad-wasm) recipe in
   `docker/wasm/sysroot.dockerfile`. No dependency on `openscad/wasm-base`.
2. **`pythonscad-wasm-python-base:local`** — adds CPython 3.14 on top of the sysroot.

`scripts/wasm-base-docker-run.sh` builds `pythonscad-wasm-python-base:local`
(sysroot + CPython in one multi-stage build) automatically if missing. It does
not tag a separate `pythonscad-wasm-sysroot:local` image.

Build the full image (sysroot + CPython; ~60 min first time, cached afterwards):

```bash
docker build -f docker/wasm/sysroot.dockerfile --target wasm-python-base \
  -t pythonscad-wasm-python-base:local .
```

To build or tag the sysroot stage only (no CPython):

```bash
docker build -f docker/wasm/sysroot.dockerfile --target wasm-sysroot \
  -t pythonscad-wasm-sysroot:local .
```

Legacy two-step build (requires sysroot image tag first):

```bash
docker build \
  -f Dockerfile.wasm-python-base \
  -t pythonscad-wasm-python-base:local .
```

- `/cpython-wasm/lib/libpython3.14.a` — static library
- `/cpython-wasm/include/python3.14/` — C headers
- `/cpython-wasm/lib/python3.14/` — stdlib (`.py` files only; heavy dirs stripped)

`scripts/wasm-base-docker-run.sh` builds the base image automatically if it is
not present, then layers a ccache image on top for fast incremental builds.

## Building

### Node variant (for testing)

```bash
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-wasm-node \
  -DWASM_BUILD_TYPE=node \
  -DCMAKE_BUILD_TYPE=Release \
  -DEXPERIMENTAL=1

./scripts/wasm-base-docker-run.sh cmake --build build-wasm-node -j$(nproc)
```

### Web variant (for browser distribution)

```bash
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-wasm-web \
  -DWASM_BUILD_TYPE=web \
  -DCMAKE_BUILD_TYPE=Release \
  -DEXPERIMENTAL=1

./scripts/wasm-base-docker-run.sh cmake --build build-wasm-web -j$(nproc)
```

The web build produces three files:

| File              | Size (approx) | Purpose                                        |
| ----------------- | ------------- | ---------------------------------------------- |
| `pythonscad.js`   | 210 KB        | ES6 module loader                              |
| `pythonscad.wasm` | 18 MB         | Binary (geometry engine + CPython)             |
| `pythonscad.data` | 17 MB         | Preloaded Python stdlib + PythonSCAD libraries |

## Smoke testing

### Node variant

```bash
docker run --rm -i \
  -v "$PWD:/src:rw" \
  -v "$PWD/libraries/python:/cpython-wasm/lib/pythonscad/libraries/python:ro" \
  -w /src/build-wasm-node \
  pythonscad-wasm-python-base:local \
  bash -c "
    echo 'from openscad import *
show(difference(cube([20,20,20], center=True), cylinder(h=25, r=5, center=True)))' \
      > /tmp/test.py
    node pythonscad.js -o /tmp/out.stl --trust-python /tmp/test.py
    wc -c /tmp/out.stl
  "
```

A successful run writes a binary STL of roughly 15 KB (80 triangular facets).

### Web variant (browser)

```bash
cp wasm-test/test.html wasm-test/notebook.html build-wasm-web/
mkdir -p build-wasm-web/vendor
cp wasm-test/vendor/three.module.min.js wasm-test/vendor/three.core.min.js build-wasm-web/vendor/
python3 wasm-test/serve.py 8080 build-wasm-web/
# Open http://localhost:8080/test.html or /notebook.html
```

The notebook page imports a vendored ES-module build of three.js
(`three.module.min.js`, which pulls in `three.core.min.js` from the same
`vendor/` dir — see `wasm-test/package.json`; run `npm ci` in `wasm-test/`
after Dependabot bumps three.js). Fonts fall back to system UI stacks — no
external CDN at runtime.

The `wasm-test/serve.py` server sets `.wasm → application/wasm` and
`.data → application/octet-stream` MIME types, which browsers require.

## JavaScript API

```javascript
import OpenSCAD from './pythonscad.js';   // ES6 module (web build)

const mod = await OpenSCAD({
  print:    (s) => console.log(s),
  printErr: (s) => console.warn(s),
});

// Write Python source to the virtual filesystem
mod.FS.writeFile('/input.py', 'from openscad import *\nshow(cube(10))');

// Render; exit() throws ExitStatus
let exitCode = 0;
try {
  mod.callMain(['-o', '/output.stl', '--trust-python', '/input.py']);
} catch (e) {
  exitCode = (e && typeof e.status === 'number') ? e.status : 1;
}

// Read result
const stl = mod.FS.readFile('/output.stl');
```

Key points:

- `INVOKE_RUN=0` is set in the CMake web build, so `main()` is **not** called
  automatically on module load — you must call `mod.callMain(...)` explicitly.
- `--trust-python` is required; without it the Python evaluator refuses to run.
- The module is not re-entrant: create a fresh instance for each render if you
  need concurrent or repeated use.

## CMake variables

| Variable              | Default         | Description                                          |
| --------------------- | --------------- | ---------------------------------------------------- |
| `WASM_BUILD_TYPE`     | `node`          | `node` or `web`                                      |
| `CPYTHON_WASM_PREFIX` | `/cpython-wasm` | Path to CPython WASM install inside the Docker image |
| `CPYTHON_WASM_PYVER`  | `3.14`          | Python version string                                |

## Known gotchas

### Docker image naming

`scripts/wasm-base-docker-run.sh` expects image name `pythonscad-wasm-python-base:local`
(dashes). If you built it manually as `pythonscad/wasm-python-base:local` (slash),
reconcile with:

```bash
docker tag pythonscad/wasm-python-base:local pythonscad-wasm-python-base:local
```

### Stale ccache layer

`pythonscad-wasm-ccache:local` is built on top of `pythonscad-wasm-python-base:local`.
If you rebuild the base image, force-rebuild the ccache layer too:

```bash
docker rmi pythonscad-wasm-ccache:local
./scripts/wasm-base-docker-run.sh true  # re-creates the ccache layer
```

### `config.pythonpath_env` is unreliable in cross-compiled builds

In `wasm32-emscripten` builds, CPython's baked-in prefix takes precedence over
`config.pythonpath_env`. PythonSCAD works around this by inserting library paths
directly into `sys.path` via the Python C API after `Py_InitializeFromConfig`:

```cpp
PyObject *syspath = PySys_GetObject("path");
PyList_Insert(syspath, 0, PyUnicode_FromString(libpath));
```

Do not revert to `config.pythonpath_env` for WASM path setup.

### Node build library path

The node variant looks for PythonSCAD libraries at
`/cpython-wasm/lib/pythonscad/libraries/python` (hardcoded, because NODERAWFS
accesses the real host filesystem). When running outside the Docker container,
mount or symlink this path to `libraries/python` in the repo root.
