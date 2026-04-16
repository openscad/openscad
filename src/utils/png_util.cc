#include "png_util.h"
#include "lodepng/lodepng.h"
#include "src/utils/printutils.h"
#include <cstring>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>

void convert_image(img_data_t& data, std::vector<uint8_t>& img, unsigned int width, unsigned int height)
{
  data.width = width;
  data.height = height;
  data.resize((size_t)width * height);
  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int x = 0; x < width; ++x) {
      long idx = 4l * (y * width + x);
      data[x + (width * (height - 1 - y))] = Vector3f(img[idx], img[idx + 1], img[idx + 2]);
    }
  }
}

bool is_png(std::vector<uint8_t>& png)
{
  const size_t pngHeaderLength = 8;
  const uint8_t pngHeader[pngHeaderLength] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
  return (png.size() >= pngHeaderLength && std::memcmp(png.data(), pngHeader, pngHeaderLength) == 0);
}

img_data_t read_png_or_dat(std::string filename)
{
  img_data_t data;
  std::vector<uint8_t> png;
  int ret_val = 0;
  try {
    ret_val = lodepng::load_file(png, filename);
  } catch (std::bad_alloc& ba) {
    LOG(message_group::Warning, "bad_alloc caught for '%1$s'.", ba.what());
    return data;
  }

  if (ret_val == 78) {
    LOG(message_group::Warning, "The file '%1$s' couldn't be opened.", filename);
    return data;
  }

  if (!is_png(png)) {
    png.clear();
    return read_dat(filename);
  }

  unsigned int width, height;
  std::vector<uint8_t> img;
  auto error = lodepng::decode(img, width, height, png);
  if (error) {
    LOG(message_group::Warning, "Can't read PNG image '%1$s'", filename);
    data.clear();
    return data;
  }

  convert_image(data, img, width, height);

  return data;
}

img_data_t read_dat(std::string filename)
{
  img_data_t data;
  std::ifstream stream(fs::u8path(filename));

  if (!stream.good()) {
    LOG(message_group::Warning, "Can't open DAT file '%1$s'.", filename);
    return data;
  }

  int lines = 0, columns = 0;
  double min_val =
    1;  // this balances out with the (min_val-1) inside createGeometry, to match old behavior

  using tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> sep(" \t");

  // We use an unordered map because the data file may not be rectangular,
  // and we may need to fill in some bits.
  using unordered_image_data_t =
    std::unordered_map<std::pair<int, int>, double, boost::hash<std::pair<int, int>>>;
  unordered_image_data_t unordered_data;

  while (!stream.eof()) {
    std::string line;
    while (!stream.eof() && (line.size() == 0 || line[0] == '#')) {
      std::getline(stream, line);
      boost::trim(line);
    }
    if (line.size() == 0 && stream.eof()) break;

    int col = 0;
    tokenizer tokens(line, sep);
    try {
      for (const auto& token : tokens) {
        auto v = boost::lexical_cast<double>(token);
        unordered_data[std::make_pair(lines, col++)] = v;
        if (col > columns) columns = col;
        min_val = std::min(v, min_val);
      }
    } catch (const boost::bad_lexical_cast& blc) {
      if (!stream.eof()) {
        LOG(message_group::Warning, "Illegal value in '%1$s': %2$s", filename, blc.what());
      }
      return data;
    }

    lines++;
  }

  data.width = columns;
  data.height = lines;

  // Now convert the unordered, possibly non-rectangular data into a well ordered vector
  // for faster access and reduced memory usage.
  data.resize((size_t)lines * columns);
  for (int i = 0; i < lines; ++i)
    for (int j = 0; j < columns; ++j) {
      auto pixel = unordered_data[std::make_pair(i, j)] * 255.0 / 100.0;
      data[i * columns + j] = Vector3f(pixel, pixel, pixel);
    }
  return data;
}
