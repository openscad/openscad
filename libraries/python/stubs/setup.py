from setuptools import setup, find_packages

setup(
    name="pythonscad-stubs",
    version="0.1.0",
    description="Type stubs for PythonSCAD",
    packages=find_packages(),
    package_data={
        "openscad": ["py.typed", "__init__.pyi"],
    },
    zip_safe=False,  # Required for type hints to work
    python_requires=">=3.11",
)