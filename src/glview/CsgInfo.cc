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
#include "io/ipc_channel.h"
#include "io/ipc_geometry.h"
#include "json/json.hpp"
#include "utils/printutils.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

json write_chain(const std::vector<CSGChainObject>& chain, const std::string& filename,
                 std::map<const PolySet *, std::string>& geometries,
                 const std::map<const PolySet *, std::string>& streamed)
{
  json output = json::array();
  for (const auto& object : chain) {
    if (!object.leaf || !object.leaf->polyset) continue;
    auto geometry = geometries.find(object.leaf->polyset.get());
    if (geometry == geometries.end()) {
      const auto already = streamed.find(object.leaf->polyset.get());
      if (already != streamed.end()) {
        // Sent during evaluation (feature 34). Reference the name it went out under rather than
        // serializing the same mesh a second time.
        geometry = geometries.emplace(object.leaf->polyset.get(), already->second).first;
      } else {
        const auto path = filename + ".leaf-" + std::to_string(geometries.size()) + kIpcGeometrySuffix;
        // Not streamed: either this is not a compute worker, or the leaf reached the products
        // without passing through the evaluator hook. Write it under the same naming scheme, so
        // the reference the products carry needs no special case at either end.
        if (ipc_payload_sink::collecting()) {
          export_ipc_geometry(*object.leaf->polyset, ipc_payload_sink::open(path));
        } else {
          std::ofstream stream(fs::u8path(path), std::ios::binary);
          export_ipc_geometry(*object.leaf->polyset, stream);
          stream.flush();
          stream.close();
        }
        geometry = geometries.emplace(object.leaf->polyset.get(), path).first;
      }
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
                    std::map<const PolySet *, std::string>& geometries,
                    const std::map<const PolySet *, std::string>& streamed)
{
  json output = json::array();
  if (!products) return output;
  for (const auto& product : products->products) {
    output.push_back(
      {{"intersections", write_chain(product.intersections, filename, geometries, streamed)},
       {"subtractions", write_chain(product.subtractions, filename, geometries, streamed)}});
  }
  return output;
}

std::vector<CSGChainObject> read_chain(const json& input,
                                       std::map<std::string, std::shared_ptr<const PolySet>>& geometries,
                                       const std::function<bool()>& continue_loading,
                                       const IpcPayloadResolver& resolve,
                                       const IpcGeometryResolver& decoded)
{
  std::vector<CSGChainObject> output;
  for (const auto& item : input) {
    if (continue_loading && !continue_loading()) return {};
    const auto path = item["geometry"].get<std::string>();
    auto geometry = geometries.find(path);
    if (geometry == geometries.end()) {
      bool continue_to_placement = false;
      // Already decoded while the worker was still evaluating the rest of the preview
      // (feature 34), so there is nothing left to do for this leaf.
      if (decoded) {
        if (auto ready = decoded(path)) {
          geometry = geometries.emplace(path, std::move(ready)).first;
          continue_to_placement = true;
        }
      }
      // Same path either way: it is the name the worker gave the payload, and also the file
      // it would have written, so nothing about how products.json refers to leaves changes.
      std::unique_ptr<PolySet> imported;
      if (!continue_to_placement && resolve) {
        if (const auto *payload = resolve(path)) {
          imported = import_ipc_geometry_buffer(payload->data(), payload->size(), path);
        }
      } else if (!continue_to_placement) {
        imported = import_ipc_geometry(path);
      }
      if (!continue_to_placement && !imported) {
        // Abort rather than emplacing a null PolySet, which renders as a silently missing
        // object. Name the leaf that failed: an empty viewport is otherwise indistinguishable
        // from a model that legitimately produced no geometry.
        LOG(message_group::Error,
            "Could not read compute worker geometry '%1$s'; the preview is incomplete.", path);
        return {};
      }
      if (!continue_to_placement) {
        if (item.contains("convexity")) {
          imported->setConvexity(item["convexity"].get<int>());
        }
        geometry = geometries.emplace(path, std::shared_ptr<const PolySet>(std::move(imported))).first;
      }
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
  const std::function<bool()>& continue_loading, const IpcPayloadResolver& resolve,
  const IpcGeometryResolver& decoded)
{
  if (input.empty()) return {};
  auto output = std::make_shared<CSGProducts>();
  output->products.clear();
  for (const auto& item : input) {
    CSGProduct product;
    product.intersections =
      read_chain(item["intersections"], geometries, continue_loading, resolve, decoded);
    product.subtractions =
      read_chain(item["subtractions"], geometries, continue_loading, resolve, decoded);
    output->products.push_back(std::move(product));
  }
  return output;
}

}  // namespace

bool CsgInfo::write_products(const std::string& filename) const
{
  std::map<const PolySet *, std::string> geometries;
  json output{
    {"root", ::write_products(root_products, filename, geometries, streamed_leaves)},
    {"highlights", ::write_products(highlights_products, filename, geometries, streamed_leaves)},
    {"background", ::write_products(background_products, filename, geometries, streamed_leaves)}};
  output["nodes"] = json::array();
  for (const auto& node : source_nodes) {
    output["nodes"].push_back({{"index", node.index},
                               {"parent", node.parent},
                               {"name", node.name},
                               {"file", node.file},
                               {"line", node.line},
                               {"column", node.column}});
  }
  if (camera_info.has_camera) {
    output["camera"] = {{"noauto", camera_info.noauto},
                        {"vpr", {camera_info.vpr[0], camera_info.vpr[1], camera_info.vpr[2]}},
                        {"vpt", {camera_info.vpt[0], camera_info.vpt[1], camera_info.vpt[2]}},
                        {"vpd", camera_info.vpd},
                        {"vpf", camera_info.vpf}};
  }
  if (ipc_payload_sink::collecting()) {
    ipc_payload_sink::open(filename) << output;
    return true;
  }
  std::ofstream stream(fs::u8path(filename));
  stream << output;
  // Close before checking: the destructor would otherwise flush after good() was read, so a
  // short write went unnoticed and the GUI was handed a truncated product list.
  stream.flush();
  stream.close();
  if (!stream.good()) {
    LOG(message_group::Error, "Could not write preview products to '%1$s'.", filename);
    return false;
  }
  return true;
}

bool CsgInfo::read_products(const std::string& filename, const std::function<bool()>& continue_loading,
                            const IpcPayloadResolver& resolve, const IpcGeometryResolver& decoded)
{
  json input;
  // A truncated or absent products file is exactly what a crashed or half-flushed worker
  // leaves behind; nlohmann throws on it, and this runs inside a Qt slot where an escaping
  // exception would take the window down instead of reporting a failed preview.
  const std::string *payload = resolve ? resolve(filename) : nullptr;
  if (resolve && !payload) {
    LOG(message_group::Error, "Compute worker returned no preview products for '%1$s'.", filename);
    return false;
  }
  std::ifstream stream;
  if (!payload) stream.open(fs::u8path(filename));
  try {
    if (payload) {
      input = json::parse(*payload);
    } else {
      stream >> input;
    }
  } catch (const json::exception& e) {
    LOG(message_group::Error, "Could not parse preview products in '%1$s': %2$s", filename, e.what());
    return false;
  }
  if (!payload && !stream.good() && !stream.eof()) {
    LOG(message_group::Error, "Could not read preview products from '%1$s'.", filename);
    return false;
  }
  std::map<std::string, std::shared_ptr<const PolySet>> geometries;
  root_products = ::read_products(input["root"], geometries, continue_loading, resolve, decoded);
  highlights_products =
    ::read_products(input["highlights"], geometries, continue_loading, resolve, decoded);
  background_products =
    ::read_products(input["background"], geometries, continue_loading, resolve, decoded);
  for (const auto& node : input.value("nodes", json::array())) {
    source_nodes.push_back(
      {node["index"], node["parent"], node["name"], node["file"], node["line"], node["column"]});
  }
  if (input.contains("camera")) {
    const auto& cam = input["camera"];
    camera_info.has_camera = true;
    camera_info.noauto = cam.value("noauto", false);
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
