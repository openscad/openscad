"""Regression fixture for ``osimport(..., split_by_color=True)`` feeding
directly into the dict form of ``export()``.

Imports a two-color SVG with ``split_by_color=True`` (returning a dict
keyed by hex color), extrudes each part, and exports the resulting dict
straight to 3MF -- exercising the exact multi-toolhead workflow this
feature exists for: one separately named/colored 3MF part per SVG
color, with no manual splitting needed on the caller's side. The
driver normalizes via ``post_process_3mf`` (XML extraction + UUID /
timestamp / namespace scrub) before the text compare.
"""
from pythonscad import export, osimport

parts = osimport("../svg/two-color.svg", split_by_color=True)
solids = {name: obj.linear_extrude(2) for name, obj in parts.items()}
export(solids, "svg-split-by-color.3mf")
