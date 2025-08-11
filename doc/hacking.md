# Coding Style

The OpenSCAD coding style is encoded in `.uncrustify.cfg`.

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

`beautify.sh` can also be used directly as a git hook.

    cd .git/hooks
    ln -s ../../scripts/beautify.sh pre-commit

After making a commit beautify will automatically run. You can then check the changes, and add them to their own commit, or amend them to the previous commit using `git commit --amend`

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
