#pragma once

#include <cstdint>

// Paper Data used by ExportPDF
enum class PaperSizes : std::uint8_t {
  A6,
  A5,
  A4,
  A3,
  LETTER,
  LEGAL,
  TABLOID,
};

// Dimensions in pts per PDF standard, used by ExportPDF
// See also: https://www.prepressure.com/library/paper-size
// rows map to paperSizes enums
// columns are Width, Height
const int paperDimensions[7][2] = {
  {298,  420}, // A6
  {420,  595}, // A5
  {595,  842}, // A4
  {842, 1190}, // A3
  {612,  792}, // Letter
  {612, 1008}, // Legal
  {792, 1224}, // Tabloid
};

enum class PaperOrientations : std::uint8_t {
  AUTO,
  PORTRAIT,
  LANDSCAPE
};
