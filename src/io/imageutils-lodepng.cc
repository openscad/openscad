#include <iostream>
#include <vector>

#include "io/imageutils.h"
#include "lodepng/lodepng.h"

bool write_png(std::ostream& output, unsigned char *pixels, int width, int height, bool with_alpha)
{
  std::vector<unsigned char> dataout;
  lodepng::State state;
  state.encoder.auto_convert = false;
  // some png renderers have different interpretations of alpha, so don't use it unless asked
  state.info_png.color.colortype = with_alpha ? LCT_RGBA : LCT_RGB;
  state.info_png.color.bitdepth = 8;
  const auto err = lodepng::encode(dataout, pixels, width, height, state);
  if (err) return false;
  output.write(reinterpret_cast<const char *>(&dataout[0]), dataout.size());
  if (output.bad()) std::cerr << "Error writing to ostream\n";
  return output.good();
}
