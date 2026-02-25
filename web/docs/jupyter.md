# Setting up a Jupyter Notebook Environment with PythonSCAD on Linux

This guide explains how to set up a clean Jupyter Notebook environment
for **PythonSCAD** on Linux, starting from a fresh Conda installation.

------------------------------------------------------------------------

## 1. Install Miniconda

Download the latest Miniconda installer:

``` bash
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
```

Run the installer:

``` bash
bash Miniconda3-latest-Linux-x86_64.sh
```

Accept the defaults and allow Conda to initialize your shell.

Restart your terminal and verify installation:

``` bash
conda --version
```

------------------------------------------------------------------------

## 2. Create a Dedicated Environment

Create a clean environment for PythonSCAD:

``` bash
conda create -n pythonscad python=3.11
```

Activate it:

``` bash
conda activate pythonscad
```

------------------------------------------------------------------------

## 3. Install Required Packages

Install core dependencies:

``` bash
conda install numpy cmake
pip install notebook ipywidgets pythreejs ipykernel
```

If using classic Jupyter Notebook (v6), enable widgets:

``` bash
jupyter nbextension enable --py widgetsnbextension --sys-prefix
```

For Notebook 7 or JupyterLab, widgets work automatically.

------------------------------------------------------------------------

## 4. Install PythonSCAD

Navigate to your PythonSCAD project directory:

``` bash
cd ~/git/pythonscad
```

Install in editable (development) mode:

``` bash
pip install -e .
```

Verify installation:

``` bash
python -c "import pythonscad; print(pythonscad.__file__)"
```

------------------------------------------------------------------------

## 5. Register the Kernel with Jupyter

Make the Conda environment available in Jupyter:

``` bash
python -m ipykernel install --user --name pythonscad --display-name "PythonSCAD"
```

Start Jupyter:

``` bash
jupyter notebook
```

Select the **PythonSCAD** kernel in the Notebook interface.

------------------------------------------------------------------------

## 6. Test 3D Rendering

In a notebook cell:

``` python
import pythonscad as ps

obj = ps.cube([10, 10, 10])
obj
```

If correctly configured:

-   The object renders as a 3D widget
-   ipywidgets display properly
-   No comm protocol errors appear

------------------------------------------------------------------------

## 7. Development Workflow

If you modify C++ sources:

``` bash
cmake --build build
```

Restart the Jupyter kernel after rebuilding.

Because PythonSCAD is installed in editable mode, no reinstall is
necessary.

------------------------------------------------------------------------

## 8. Troubleshooting

### Widgets appear as text (e.g. `IntSlider(value=0)`)

Check:

``` bash
jupyter nbextension list
```

If disabled:

``` bash
jupyter nbextension enable --py widgetsnbextension --sys-prefix
```

------------------------------------------------------------------------

### Wrong Python environment

Verify:

``` bash
which python
which jupyter
```

Both must point inside:

    miniconda3/envs/pythonscad/

Inside a notebook:

``` python
import sys
print(sys.executable)
```

------------------------------------------------------------------------

## Recommended Project Layout

    pythonscad/
    ├── pyproject.toml
    ├── CMakeLists.txt
    └── src/
        └── pythonscad/
            ├── __init__.py
            ├── jupyterdisplay.py
            └── _core.so

This ensures clean imports like:

``` python
import pythonscad.jupyterdisplay
```

------------------------------------------------------------------------

# Done

You now have:

-   An isolated Conda environment
-   Jupyter Notebook
-   ipywidgets and pythreejs
-   PythonSCAD installed in development mode
-   Working 3D rendering support
