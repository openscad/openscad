#include "glview/CsgInfo.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <string>

#include "geometry/PolySet.h"
#include "io/export.h"
#include "io/import.h"
#include "json/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

json write_chain(const std::vector<CSGChainObject>& chain, const std::string& filename,
                 std::map<const PolySet *, std::string>& geometries)
{
  json output = json::array();
  for (const auto& object : chain) {
    auto geometry = geometries.find(object.leaf->polyset.get());
    if (geometry == geometries.end()) {
      const auto path = filename + ".leaf-" + std::to_string(geometries.size()) + ".off";
      std::ofstream stream(fs::u8path(path));
      stream << std::setprecision(std::numeric_limits<double>::max_digits10);
      export_off(object.leaf->polyset, stream);
      geometry = geometries.emplace(object.leaf->polyset.get(), path).first;
    }

    json matrix = json::array();
    for (int row = 0; row < 4; ++row) {
      for (int column = 0; column < 4; ++column) {
        matrix.push_back(object.leaf->matrix.matrix()(row, column));
      }
    }
    output.push_back({{"geometry", geometry->second},
                      {"matrix", std::move(matrix)},
                      {"color",
                       {object.leaf->color.r(), object.leaf->color.g(), object.leaf->color.b(),
                        object.leaf->color.a()}},
                      {"label", object.leaf->label},
                      {"index", object.leaf->index},
                      {"flags", object.flags}});
  }
  return output;
}

json write_products(const std::shared_ptr<CSGProducts>& products, const std::string& filename,
                    std::map<const PolySet *, std::string>& geometries)
{
  json output = json::array();
  if (!products) return output;
  for (const auto& product : products->products) {
    output.push_back({{"intersections", write_chain(product.intersections, filename, geometries)},
                      {"subtractions", write_chain(product.subtractions, filename, geometries)}});
  }
  return output;
}

std::vector<CSGChainObject> read_chain(const json& input,
                                       std::map<std::string, std::shared_ptr<const PolySet>>& geometries)
{
  std::vector<CSGChainObject> output;
  for (const auto& item : input) {
    const auto path = item["geometry"].get<std::string>();
    auto geometry = geometries.find(path);
    if (geometry == geometries.end()) {
      auto imported = import_off(path, Location::NONE);
      geometry = geometries.emplace(path, std::shared_ptr<const PolySet>(std::move(imported))).first;
    }
    Transform3d matrix = Transform3d::Identity();
    for (int row = 0; row < 4; ++row) {
      for (int column = 0; column < 4; ++column) {
        matrix.matrix()(row, column) = item["matrix"][row * 4 + column].get<double>();
      }
    }
    const auto& color = item["color"];
    auto leaf = std::make_shared<CSGLeaf>(geometry->second, matrix,
                                          Color4f(color[0].get<float>(), color[1].get<float>(),
                                                  color[2].get<float>(), color[3].get<float>()),
                                          item["label"].get<std::string>(), item["index"].get<int>());
    output.emplace_back(leaf, static_cast<CSGNode::Flag>(item["flags"].get<unsigned int>()));
  }
  return output;
}

std::shared_ptr<CSGProducts> read_products(
  const json& input, std::map<std::string, std::shared_ptr<const PolySet>>& geometries)
{
  if (input.empty()) return {};
  auto output = std::make_shared<CSGProducts>();
  output->products.clear();
  for (const auto& item : input) {
    CSGProduct product;
    product.intersections = read_chain(item["intersections"], geometries);
    product.subtractions = read_chain(item["subtractions"], geometries);
    output->products.push_back(std::move(product));
  }
  return output;
}

}  // namespace

bool CsgInfo::write_products(const std::string& filename) const
{
  std::map<const PolySet *, std::string> geometries;
  json output{{"root", ::write_products(root_products, filename, geometries)},
              {"highlights", ::write_products(highlights_products, filename, geometries)},
              {"background", ::write_products(background_products, filename, geometries)}};
  std::ofstream stream(fs::u8path(filename));
  stream << output;
  return stream.good();
}

bool CsgInfo::read_products(const std::string& filename)
{
  std::ifstream stream(fs::u8path(filename));
  json input;
  stream >> input;
  if (!stream.good() && !stream.eof()) return false;
  std::map<std::string, std::shared_ptr<const PolySet>> geometries;
  root_products = ::read_products(input["root"], geometries);
  highlights_products = ::read_products(input["highlights"], geometries);
  background_products = ::read_products(input["background"], geometries);
  return true;
}
