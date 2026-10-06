# Using libraries in your designs

Install third-party PythonSCAD packages into a virtual environment, then
`import` them like any other Python module. Authors package libraries as
described in [Creating and publishing a library](creating.md).

Remember that the **pip name** may differ from the **import name**. Example:
`pip install pythonscad-pincutout` installs a package you import as `pincutout`.

## GUI: File → Python virtual environment

Desktop builds can point PythonSCAD at a project venv so installed packages are
visible to your scripts.

1. **File → Python → Create Virtual Environment…** — choose an empty directory
   (often `.venv` next to your design), or **Select Virtual Environment…** and
   pick a directory that already contains `pyvenv.cfg`.
2. Restart PythonSCAD when prompted. After restart, that environment is used for
   imports.
3. Install packages into the same environment with that venv's `pip`.

PythonSCAD looks for executables under either `bin/` or `Scripts/` in the
selected venv (POSIX layouts and plain Windows venvs use one or the other;
MSYS2 environments often use `bin/`). Use whichever directory exists:

=== "Linux / macOS"

    ```shell
    # From a terminal, using the venv you selected in PythonSCAD:
    path/to/your/.venv/bin/pip install pythonscad-pincutout
    ```

=== "Windows (Python.org / Store venv)"

    ```powershell
    # Typical native Windows venv layout:
    path\to\your\.venv\Scripts\pip.exe install pythonscad-pincutout
    ```

=== "Windows (MSYS2 / bin layout)"

    ```bash
    # Packaged MSYS2 builds often create a POSIX-style bin/ directory:
    path/to/your/.venv/bin/pip install pythonscad-pincutout
    ```

You can also activate the venv first (`source .venv/bin/activate` or
`.\.venv\Scripts\Activate.ps1`) and run `python -m pip install …`.

4. In your design:

```python
from pythonscad import *
import pincutout

cutter = pincutout.PinCutout.generic_25mm().render()
show(cube([40, 40, 2]) - cutter)
```

A minimal consumer that uses this library is
[unpolitischer-verein-button](https://github.com/nomike/unpolitischer-verein-button).

## Headless / IDE: venv + `pip install pythonscad`

For CLI rendering, editors, or CI, create a venv and install both PythonSCAD and
your libraries. This matches the
[project-local environment](../installation.md#project-local-environment-for-ide-support)
setup, with extra dependencies for geometry libraries.

=== "Linux / macOS"

    ```shell
    python3 -m venv .venv
    . .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install pythonscad pythonscad-pincutout
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    python -m pip install --upgrade pip
    python -m pip install pythonscad pythonscad-pincutout
    ```

Record dependencies in `requirements.txt`:

```text
pythonscad==1.1.2
pythonscad-pincutout==0.1.0
```

```shell
python -m pip install -r requirements.txt
```

Or list them under `[project] dependencies` in a `pyproject.toml` and install
with `python -m pip install -e .`.

Point the GUI at the same `.venv` (**File → Python → Select Virtual
Environment…**) if you edit in PythonSCAD and want identical packages.

## Version pinning and supply-chain hygiene

Pin exact versions (or use a lock file) in design repositories so a new upstream
release cannot silently change geometry or ship unexpected code:

```text
# requirements.txt
pythonscad==1.1.2
pythonscad-pincutout==0.1.0
```

```toml
# pyproject.toml excerpt
dependencies = [
  "pythonscad==1.1.2",
  "pythonscad-pincutout==0.1.0",
]
```

Lock files (`uv.lock`, hashed `requirements.txt` from pip-tools, and similar)
give stronger reproducibility. Prefer them for shared or published designs.

Pins go stale without updates. Use an automated dependency bot so you review
bumps on your schedule instead of forgetting them:

- [Dependabot](https://docs.github.com/en/code-security/dependabot) (GitHub)
- [Renovate](https://docs.renovatebot.com/) (GitHub, GitLab, and others)
- GitLab also documents
  [dependency scanning](https://docs.gitlab.com/ee/user/application_security/dependency_scanning/)
  and related dependency-update workflows

Configure the tool for the PyPI ecosystem so pull/merge requests propose version
bumps; merge when you have checked that renders still look correct.
