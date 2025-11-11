## Definition of Terms

* "Calculated RGB" or "calculated alpha": the module `color` has a few ways to specify its inputs. These are the RGB and alpha (respectively) after parsing the arguments.
  * A "valid RGB" is a list of 3 real numbers where all three values are neutral or positive and 0.0-1.0 is considered the full range of intensity, with 0.0 being none and 1.0 being the maximum. The ordering is Red, Green, Blue.
  * A "valid alpha" is a neutral or positive real number where 0.0-1.0 is considered the full range, with 0.0 being completely transparent and 1.0 being completely opaque.
* "Color" is all of red, green, blue, and alpha values. It consists of two parts: RGB and alpha.
  * It can be partially uncolored: alpha could be valid and RGB could be uncolored.
* "`color`" refers to the module `color`.
* "Default color", without further qualifiers, is the "default positive face color". For rendering and export, this means `CGAL_FACE_FRONT_COLOR` of the currently-selected theme; for preview, this is the `OPENCSG_FACE_FRONT_COLOR`.
* "Default negative color", without further qualifiers, is the "default negative face color". For rendering and export, this means `CGAL_FACE_BACK_COLOR` of the currently-selected theme; for preview, this is the `OPENCSG_FACE_BACK_COLOR`.
* "Model export" is any export format containing coordinates for models and not rasterized output.
* "Negative color": when `difference()` removes from the first child geometry, it colors the new faces with negative color.
* "RGB" refers to red, green, and blue values commonly used in computer graphics for displaying light in the human visual spectrum. The colloquial English definition of color is mostly captured by RGB.
* "RGBA" is a list of 4 numbers; the first three being RGB and the last element being alpha. It is everything needed to define a color.
* "Uncolored" without qualifiers refers to any of "uncolored positive" and "uncolored negative".
  * "Uncolored positive" is the color of any newly-created face not from `difference()`.
  * "Uncolored negative" is the color of a newly-created face created by `difference()` when the removing object's face is uncolored.
* "Viewport" is the interactable section of UI containing the preview or render view.

## Interpretation

* The module instantiation of `color`:
  * Must affect either all or no faces of child geometry with the same transformation.
  * If setting any RGB value, must set all three of red, green, and blue values.
  * If it calculates a valid RGB and invalid alpha, it shall apply the calculated RGB to all child geometry and must not affect the alpha of any child geometry; and vice-versa.
  * If it calculates valid RGB and valid alpha, it shall apply the calculated RGB and alpha to all child geometry.
  * If it calculates invalid RGB and invalid alpha, it shall not affect child geometry.
* For any RGB used by the implementation, one of the following must be true about the individual R, G, and B values:
  * All of the values must be meet the valid criteria.
  * All of the values must be the same uncolored sentinel.
* In the viewport, lighting may affect the color of the faces. The faces in model exports must not be affected by lighting.
* Preview shall show transparency. Render may show transparency.
  * Some export formats support transparency (i.e. PNG) and some do not (i.e. 3MF) and some do not support any color (STL).
  * If an export format supports transparency, the software should use transparency when exporting unless directed by the user otherwise.
  * The software may allow the user to override or ignore any transparency and/or coloring when exporting.
  * Objects with the same color but different alpha shall be considered the same color in formats which support color and not transparency. Alpha of zero shall be considered the associated color.
* Modifier color effects must not affect rendering or export.
* As scad evaluation ceases, any uncolored faces may remain uncolored.
* At the time of preview, render, or export, any partially- or fully-uncolored faces must be reified using a color scheme.
  * Internal sentinel values, if any, for uncolored shall not be exposed to the user.
  * Valid RGB or valid alpha must not be changed. Every uncolored RGB or alpha must be changed to valid values.
  * For preview and render, the software should use the current color scheme.
  * For export, the software may use the current color scheme or the one at the time of rendering, but must make the same decision for all exporters.
* If the color scheme is changed, the viewport must be updated to the new color scheme.  Uncolored portions of the model may be updated to the new color scheme.
* The negative color shall use valid parts (RGB or alpha) of the color of the face from the removal geometry causing the new face; for an invalid RGB or alpha, uncolored negative shall be used.
* Any of RGBA missing from a color in the color scheme shall use the maximum value from the full range of intensity for the corresponding valid definition when reifying an uncolored face.

## Argument Parsing

* Out-of-bounds numerical values shall give a warning when parameter range warnings are not suppressed.
  * For any of R, G, B, or alpha above 1.0, the calculated color or alpha may continue using that value or may clamp the value to 1.0.
* The keyword argument `alpha` to `color` shall be the sole source of alpha if not equal to `undef`.
* `color` shall take two arguments: `c` and `alpha`
* If `c` is not one of the following, `color` shall give a warning when parameter range warnings are not suppressed.
  * A list of four floating-point values `[r, g, b, a]`
  * A list of three floating-point values `[r, g, b]`
  * A string `"#hexvalue"` in either RGB or RGBA format, and the values must all be given as one or two hexadecimal digits: `#rgb`, `#rgba`, `#rrggbb`, or `#rrggbbaa`.
  * A string `"colorname"` where colorname is from the World Wide Web Consortium's [SVG color list](http://www.w3.org/TR/css3-color/) and is used for RGB; or `"transparent"`, which has RGBA of `[0,0,0,0]`.
  * `undef`
