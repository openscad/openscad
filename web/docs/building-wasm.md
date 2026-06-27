# Building for WebAssembly

PythonSCAD can be compiled to WebAssembly with [Emscripten](https://emscripten.org/),
producing a headless build that runs the geometry kernel and an embedded
CPython interpreter directly in the browser. This powers the in-browser
[Playground](https://www.pythonscad.org/playground/).

If you only want to *try* PythonSCAD in your browser, you do not need to build
anything — just open the Playground. This page is for contributors who want to
build or hack on the WASM bundle.

## Prerequisites

The build runs inside Docker so you do not need a host Emscripten/CPython
toolchain. You need Docker and a checkout of the repository (with submodules).

There are two build variants:

- **`node`** — runs under Node.js with direct filesystem access; used for fast
  smoke tests (renders an STL via the command line).
- **`web`** — the browser bundle (`pythonscad.js` + `pythonscad.wasm` +
  `pythonscad.data`) loaded as an ES module.

## Build

```bash
# Build the base image (Emscripten + cross-compiled CPython). First run is slow
# (~1 h); cached afterwards. scripts/wasm-base-docker-run.sh builds it on demand.
docker build -f docker/wasm/sysroot.dockerfile --target wasm-python-base \
  -t pythonscad-wasm-python-base:local .

# Node variant (smoke test)
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-wasm-node \
  -DWASM_BUILD_TYPE=node -DCMAKE_BUILD_TYPE=Release -DEXPERIMENTAL=1
./scripts/wasm-base-docker-run.sh cmake --build build-wasm-node -j"$(nproc)"
node build-wasm-node/pythonscad.js -o out.stl --trust-python script.py

# Web variant (browser bundle)
./scripts/wasm-base-docker-run.sh emcmake cmake -B build-wasm-web \
  -DWASM_BUILD_TYPE=web -DCMAKE_BUILD_TYPE=Release -DEXPERIMENTAL=1
./scripts/wasm-base-docker-run.sh cmake --build build-wasm-web -j"$(nproc)"
```

## Testing the web build locally

Browsers require the right MIME types for `.wasm` (`application/wasm`) and the
preloaded `.data` package, so a plain `python3 -m http.server` will not work.
Use the bundled dev server, which sets the correct types:

```bash
cp wasm-test/test.html wasm-test/notebook.html build-wasm-web/
mkdir -p build-wasm-web/vendor
cp wasm-test/vendor/three.module.min.js wasm-test/vendor/three.core.min.js build-wasm-web/vendor/
python3 wasm-test/serve.py 8080 build-wasm-web/
# Open http://localhost:8080/test.html  or  /notebook.html
```

`test.html` exercises the command-line export path; `notebook.html` is the
interactive notebook (the same page served as the Playground).

## JavaScript API

To embed PythonSCAD in your own page, see the
[Browser / WASM API](reference/wasm-api.md) reference.

## More detail

The full build guide — base-image internals, the `node` vs `web` rationale,
JSPI/`MAIN_MODULE` notes, and known gotchas — lives in the developer docs at
[`doc/wasm-build.md`](https://github.com/pythonscad/pythonscad/blob/master/doc/wasm-build.md).
