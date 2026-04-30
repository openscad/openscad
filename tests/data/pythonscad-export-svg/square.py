"""Regression fixture for the in-script ``export()`` function: 2D SVG.

Hits the 2D branch of the export pipeline -- ``fileformat::is2D()``
returns true for ``FileFormat::SVG``. Geometry is a single
axis-aligned square so the resulting path data is short and
deterministic under ``--enable=predictible-output``. The driver runs
``post_process_progname`` on the file before the normalized text
compare.
"""
from pythonscad import square, export

export(square(10), "square.svg")
