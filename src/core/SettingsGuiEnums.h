#pragma once

#include <cstdint>

/*
 * This collects the settings enums used in GUI code. None of this must
 * depend on any GUI or Qt classes.
 */

enum class ColorListFilterType : std::uint8_t {
  fixed,
  wildcard,
  regexp,
};

enum class ColorListSortType : std::uint8_t {
  alphabetical,
  by_color,
  by_color_warmth,
  by_lightness,
};
