# Creating and publishing a library

Package reusable PythonSCAD geometry as a normal Python project. Consumers then
install it with `pip` — see [Using libraries](using.md).

## Minimal package layout

Use a **src layout** and a `pyproject.toml` (no `setup.py` needed for a
pure-Python library):

```text
my-widget/
├── pyproject.toml
├── README.md
├── LICENSE
└── src/
    └── mywidget/
        ├── __init__.py
        └── _model.py
```

### `pyproject.toml`

```toml
[build-system]
requires = ["hatchling>=1.27"]
build-backend = "hatchling.build"

[project]
name = "pythonscad-mywidget"   # pip / PyPI name
version = "0.1.0"
description = "Reusable PythonSCAD widget"
readme = "README.md"
requires-python = ">=3.10"
license = "BSD-3-Clause"
dependencies = [
  "pythonscad>=1.1.0",
]

[project.urls]
Homepage = "https://github.com/YOU/pythonscad-mywidget"
Repository = "https://github.com/YOU/pythonscad-mywidget"

[tool.hatch.build.targets.wheel]
packages = ["src/mywidget"]
```

Hatchling is a common default build backend; setuptools, Flit, and others also
work. See [Writing your pyproject.toml](https://packaging.python.org/en/latest/guides/writing-pyproject-toml/).

### Example module

`src/mywidget/_model.py`:

```python
from pythonscad import cube


def make_widget(size=10):
    """Return a simple solid callers can place, color, or boolean with."""
    return cube(size)
```

`src/mywidget/__init__.py`:

```python
from ._model import make_widget

__all__ = ["make_widget"]
```

Local check from the project root (with a venv that has build deps):

```shell
python -m pip install -e .
python -c "import mywidget; print(mywidget.make_widget())"
```

## Publishing to PyPI (overview)

You do not need a long-lived PyPI API token. Preferred approach in 2026:

1. Create the project on [TestPyPI](https://test.pypi.org/) / [PyPI](https://pypi.org/)
   and configure a
   [Trusted Publisher](https://docs.pypi.org/trusted-publishers/)
   (GitHub Actions or GitLab CI OIDC).
2. Build with [`python -m build`](https://packaging.python.org/en/latest/tutorials/packaging-projects/)
   (or `uv build`) in CI.
3. Upload with
   [`pypa/gh-action-pypi-publish`](https://github.com/pypa/gh-action-pypi-publish)
   on GitHub, or the GitLab flow described under
   [Adding a Trusted Publisher](https://docs.pypi.org/trusted-publishers/adding-a-publisher/).

Official guides (read these for the full procedure):

- [Packaging Python Projects](https://packaging.python.org/en/latest/tutorials/packaging-projects/)
- [Publishing package distributions with GitHub Actions](https://packaging.python.org/en/latest/guides/publishing-package-distribution-releases-using-github-actions-ci-cd-workflows/)
- [Using TestPyPI](https://packaging.python.org/en/latest/guides/using-testpypi/)
- [Trusted Publishers on PyPI](https://docs.pypi.org/trusted-publishers/)

For a complete PythonSCAD-oriented example (release-please, TestPyPI + PyPI
environments, hatchling src layout), see
[pythonscad-pincutout](https://github.com/nomike/pythonscad-pincutout)
(`PUBLISHING.md` and `.github/workflows/publish.yml`).
