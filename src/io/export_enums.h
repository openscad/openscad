#pragma once

#include <cstdint>

enum class ExportPdfPaperSize : std::uint8_t {
  A6,
  A5,
  A4,
  A3,
  LETTER,
  LEGAL,
  TABLOID,
};

enum class ExportPdfPaperOrientation : std::uint8_t {
  AUTO,
  PORTRAIT,
  LANDSCAPE,
};

enum class Export3mfColorMode : std::uint8_t {
  model,
  none,
  selected_only,
};

// https://github.com/3MFConsortium/spec_core/blob/master/3MF%20Core%20Specification.md:
// micron, millimeter, centimeter, inch, foot, and meter
enum class Export3mfUnit : std::uint8_t {
  micron,
  millimeter,
  centimeter,
  meter,
  inch,
  foot,
};

enum class Export3mfMaterialType : std::uint8_t {
  color,
  basematerial,
};