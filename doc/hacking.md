# Coding Style

The OpenSCAD coding style is encoded in `.clang-format`.

Coding style highlights:

* Use 2 spaces for indentation
* Use C++11 functionality where applicable. Please read Scott Meyer's Effective Modern C++ for a good primer on modern C++ style and features: https://shop.oreilly.com/product/0636920033707.do

## Beautifying code

Code to be committed can be beautified by installing `clang-format` and running
`./scripts/beautify.sh`. This will, by default, beautify all files that
are currently changed.

Alternatively, it's possible to beautify the entire codebase by running `./scripts/beautify.sh --all`.

All pull requests must pass `./scripts/beautify.sh --check` . In rare cases beautify may need to be run multiple times before all issues are resolved. If there is an issue with the local version of `clang-format` conflicting with the workflow version, the workflows output a patch that can be manually applied to resolve the differences. The patch can be pulled from GitHub or generated locally using `act`:

    act -j Beautify --artifact-server-path build/artifacts/
    unzip build/artifacts/1/beautify-patch/beautify-patch.zip
    git apply beautify.patch

This can be done automatically using `./scripts/hard_beautify.sh`, which also functions as a git hook. 

## pre-commit

OpenSCAD supports [pre-commit](https://pre-commit.com) which runs
clang-format and a few other tools on code you want to commit.

It is available in all major Linux distributions or can be installed
via `pip install pre-commit`.

Immediately after cloning the repository, run `pre-commit install`
once, to install the necessary git hook.

From then on, every time you commit changes, pre-commit will execute
some hooks which will complain if there are issues for example with
the code-styling. Minor things will be fixed automatically, but the
commit will still be aborted. You should review those automated fixes
and address any additional findings.  Stage those changes and commit
again.

If you want to run the hooks without committing, you can run
`pre-commit run`. It will only check the staged versions of files
though.

# Regression Tests

See `testing.txt`

# How to add new function/module

* Implement
* Add test
* Modules: Add example
* Document:
   * wikibooks
   * cheatsheet
   * Modules: tooltips (Editor.cc)
   * External editor modes
   * Add to RELEASE_NOTES.md
