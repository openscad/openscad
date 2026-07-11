# PythonSCAD v0.20.0 released

Hi everyone,

We’re happy to announce **PythonSCAD v0.20.0**. The headline change is **session management**: the app can remember how
you left off and bring that workspace back on the next launch. This is similar to what applications like NotedPad++ and
Visual Studio Code are doing. We’ve also improved reliability in a few places, rounded out Python API documentation,
and made small quality-of-life updates. Full technical notes are in the changelog on the
[release PR](https://github.com/pythonscad/pythonscad/pull/536).

## Session management (new)

**v0.20.0 introduces session persistence.** When you quit and reopen PythonSCAD, you can get back your **open tabs and
editors**, **multi-window layout**, **customizer state**, **view and find state**, and **per-tab language**—so routine
work is easier to resume without rebuilding your environment from scratch.

The feature also includes **autosave-style crash recovery**, a **clearer flow when closing tabs with unsaved changes**,
and a **session option in Advanced preferences** if you want to control the behavior. **Customizer** tweaks can apply
**without always pressing F5**, and the stack includes **dry-run safety** so heavy preview work is harder to trigger by
accident.

All of this ships as one coherent capability (see [PR #415](https://github.com/pythonscad/pythonscad/pull/415)).

***Note:** To go back to the old behavior, you can go to **Edit > Preferences > Advanced** and disable the **session
management option**. We tested this new feature quite extensively, but any feedback or
[bug reports](https://github.com/pythonscad/pythonscad/issues/new?template=bug_report.md) on this new feature will be
highly appreciated.*

## Improved unsaved-files dialog

When you want to close PythonSCAD a new dialog will list unsaved tabs and allow you to individually save them or decide
to not save the remaining unsaved changes.

## Save dialogs and editor language

**Save** and **Save As** now match the **editor language** you’re in: suggested names and filters line up with
**`.py` vs `.scad`**, including sensible default folders so native file dialogs behave reliably. This addresses
incorrect defaults some people hit in Python mode ([#534](https://github.com/pythonscad/pythonscad/issues/534)).

Thanks **[@bereldhuin](https://github.com/bereldhuin)** for reporting this.

## Reliability and polish

- **Context menu / right-click** handling is more robust, with fewer crashes in edge cases.
- The **measure** tool again shows **correct vertex indices** when you need them.
- **Windows** builds use the **PythonSCAD logo** in the application and nightly resources
  ([#543](https://github.com/pythonscad/pythonscad/pull/543),
  [#544](https://github.com/pythonscad/pythonscad/pull/544)).

## Documentation

- The **[Python API cheatsheet](https://pythonscad.org/cheatsheet/) and reference** are brought to a **complete**
  state ([PR #531](https://github.com/pythonscad/pythonscad/pull/531)).

## Upstream changes

Of course we merged all changes from OpenSCAD into our codebase again to get the latest bugfixes and enhancement from
our upstream project.

## Downloads

Pre-built packages are available for Linux (AppImage, `.deb`, `.rpm`), macOS (DMG), and Windows (installer and zip):

**<https://pythonscad.org/downloads/>**

As always, we’d love to hear from you—feedback and bug reports are welcome!
