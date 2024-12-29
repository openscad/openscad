#pragma once

#include <functional>
#include <string_view>

// Functions to draw ASCII text in the Hershey simplex font with
// user-provided draw function

namespace hershey {

// Determine the width of the text if drawn with DrawText()
float TextWidth(std::string_view str, float size);

// Horizontal alignment
enum class TextAlign { kLeft, kCenter, kRight };

// Draw a text at position (tx,ty) with the given alignment and size,
// output is sent to the 2D output 'draw()' function that receives.
//   "do_line"  - a boolean saying if we should line or move to the position.
//   "x", "y"   - the position to moveto/lineto
// The function makes it independent of any output device and easy to
// adapt in any environment including 3D projection.
void DrawText(std::string_view str, float tx, float ty, TextAlign align,
              float size,
              const std::function<void(bool pen_down, float x, float y)>& draw);
}  // namespace hershey
