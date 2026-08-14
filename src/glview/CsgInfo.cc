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
    if (!object.leaf || !object.leaf->polyset) continue;
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
                      {"convexity", object.leaf->polyset ? object.leaf->polyset->getConvexity() : 1},
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
                                       std::map<std::string, std::shared_ptr<const PolySet>>& geometries,
                                       const std::function<bool()>& continue_loading)
{
  std::vector<CSGChainObject> output;
  for (const auto& item : input) {
    if (continue_loading && !continue_loading()) return {};
    const auto path = item["geometry"].get<std::string>();
    auto geometry = geometries.find(path);
    if (geometry == geometries.end()) {
      auto imported = import_off(path, Location::NONE);
      if (imported && item.contains("convexity")) {
        imported->setConvexity(item["convexity"].get<int>());
      }
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
  const json& input, std::map<std::string, std::shared_ptr<const PolySet>>& geometries,
  const std::function<bool()>& continue_loading)
{
  if (input.empty()) return {};
  auto output = std::make_shared<CSGProducts>();
  output->products.clear();
  for (const auto& item : input) {
    CSGProduct product;
    product.intersections = read_chain(item["intersections"], geometries, continue_loading);
    product.subtractions = read_chain(item["subtractions"], geometries, continue_loading);
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
  output["nodes"] = json::array();
  for (const auto& node : source_nodes) {
    output["nodes"].push_back({{"index", node.index}, {"parent", node.parent}, {"name", node.name},
                               {"file", node.file}, {"line", node.line}, {"column", node.column}});
  }
  if (camera_info.has_camera) {
    output["camera"] = {
      {"vpr", {camera_info.vpr[0], camera_info.vpr[1], camera_info.vpr[2]}},
      {"vpt", {camera_info.vpt[0], camera_info.vpt[1], camera_info.vpt[2]}},
      {"vpd", camera_info.vpd},
      {"vpf", camera_info.vpf}
    };
  }
  std::ofstream stream(fs::u8path(filename));
  stream << output;
  return stream.good();
}

bool CsgInfo::read_products(const std::string& filename,
                            const std::function<bool()>& continue_loading)
{
  std::ifstream stream(fs::u8path(filename));
  json input;
  stream >> input;
  if (!stream.good() && !stream.eof()) return false;
  std::map<std::string, std::shared_ptr<const PolySet>> geometries;
  root_products = ::read_products(input["root"], geometries, continue_loading);
  highlights_products = ::read_products(input["highlights"], geometries, continue_loading);
  background_products = ::read_products(input["background"], geometries, continue_loading);
  for (const auto& node : input.value("nodes", json::array())) {
    source_nodes.push_back({node["index"], node["parent"], node["name"], node["file"], node["line"],
                            node["column"]});
  }
  if (input.contains("camera")) {
    const auto& cam = input["camera"];
    camera_info.has_camera = true;
    camera_info.vpr[0] = cam["vpr"][0].get<double>();
    camera_info.vpr[1] = cam["vpr"][1].get<double>();
    camera_info.vpr[2] = cam["vpr"][2].get<double>();
    camera_info.vpt[0] = cam["vpt"][0].get<double>();
    camera_info.vpt[1] = cam["vpt"][1].get<double>();
    camera_info.vpt[2] = cam["vpt"][2].get<double>();
    camera_info.vpd = cam.value("vpd", 0.0);
    camera_info.vpf = cam.value("vpf", 0.0);
  }
  return !continue_loading || continue_loading();
}
