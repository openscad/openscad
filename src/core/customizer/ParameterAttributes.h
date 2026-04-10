#pragma once

#include <cstddef>
#include <boost/optional.hpp>

// Temporary attribute bag populated during object expression parsing.
struct RawAttributes {
  // NumberParameter
  boost::optional<bool> sliderEnabled;

  boost::optional<double> minimum;
  boost::optional<double> maximum;
  boost::optional<double> step;

  // StringParameter
  boost::optional<bool> multiLine;    // todo
  boost::optional<size_t> maxLength;  // todo
  boost::optional<bool> color;        // todo
};
