#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>

#include "core/CSGNode.h"
#include "io/ipc_channel.h"
#include "io/ipc_geometry.h"
#include "core/CSGTreeEvaluator.h"
#include "core/Tree.h"
#include "geometry/GeometryEvaluator.h"
#include "glview/preview/CSGTreeNormalizer.h"
#include "glview/RenderSettings.h"
#include "utils/printutils.h"

/*
   Small helper class for compiling and normalizing node trees into CSG products
 */
class CsgInfo
{
public:
  struct SourceNode {
    int index;
    int parent;
    std::string name;
    std::string file;
    int line;
    int column;
  };
  struct CameraInfo {
    bool has_camera = false;
    bool noauto = false;
    double vpr[3] = {0, 0, 0};
    double vpt[3] = {0, 0, 0};
    double vpd = 0;
    double vpf = 0;
  };
  CsgInfo() = default;
  std::shared_ptr<class CSGProducts> root_products;
  std::shared_ptr<CSGProducts> highlights_products;
  std::shared_ptr<CSGProducts> background_products;
  std::vector<SourceNode> source_nodes;
  CameraInfo camera_info;

  bool write_products(const std::string& filename) const;

  // Names of leaf payloads already sent over the response channel during evaluation, keyed by
  // PolySet identity (feature 34). Empty outside a compute worker, in which case write_products
  // serializes the leaves itself as it always did.
  std::map<const PolySet *, std::string> streamed_leaves;
  // `resolve`, when set, supplies payload bytes by name instead of reading them from disk --
  // the products themselves and every leaf they refer to. That is how a compute worker's
  // preview arrives now (see io/ipc_channel.h); an empty resolver reads files as before.
  bool read_products(const std::string& filename, const std::function<bool()>& continue_loading = {},
                     const IpcPayloadResolver& resolve = {});

  // `products_file`, when set and when a compute worker is collecting payloads, makes each leaf
  // be sent as soon as its geometry is evaluated rather than after the whole walk finishes. The
  // names assigned here are what write_products then references, so the two stay consistent.
  bool compile_products(const Tree& tree, size_t normalization_limit = 0,
                        const std::string& products_file = {})
  {
    auto& root_node = tree.root();
    const auto collect_source_nodes = [this](const auto& self, const auto& node, int parent) -> void {
      const auto& location = node->modinst ? node->modinst->location() : Location::NONE;
      source_nodes.push_back({node->index(), parent, node->verbose_name(), location.fileName(),
                              location.firstLine(), location.firstColumn()});
      for (const auto& child : node->getChildren()) self(self, child, node->index());
    };
    collect_source_nodes(collect_source_nodes, root_node, -1);
    GeometryEvaluator geomevaluator(tree);
    CSGTreeEvaluator evaluator(tree, &geomevaluator);
    if (!products_file.empty() && ipc_payload_sink::collecting()) {
      evaluator.setLeafCallback([this, &products_file](const std::shared_ptr<const PolySet>& ps) {
        // Deduplicated by PolySet identity, exactly as write_chain does, so a mesh shared by
        // several leaves is sent once and referenced many times.
        if (this->streamed_leaves.count(ps.get())) return;
        const auto name =
          products_file + ".leaf-" + std::to_string(this->streamed_leaves.size()) + kIpcGeometrySuffix;
        export_ipc_geometry(*ps, ipc_payload_sink::open(name));
        this->streamed_leaves.emplace(ps.get(), name);
      });
    }
    const std::shared_ptr<CSGNode> csgRoot = evaluator.buildCSGTree(*root_node);
    std::vector<std::shared_ptr<CSGNode>> highlightNodes = evaluator.getHighlightNodes();
    std::vector<std::shared_ptr<CSGNode>> backgroundNodes = evaluator.getBackgroundNodes();

    LOG("Compiling design (CSG Products normalization)...");
    CSGTreeNormalizer normalizer(normalization_limit ? normalization_limit
                                                     : RenderSettings::inst()->openCSGTermLimit);
    if (csgRoot) {
      const std::shared_ptr<CSGNode> normalizedRoot = normalizer.normalize(csgRoot);
      if (normalizedRoot) {
        this->root_products = std::make_shared<CSGProducts>();
        this->root_products->import(normalizedRoot);
        LOG("Normalized CSG tree has %1$d elements", int(this->root_products->size()));
      } else {
        this->root_products.reset();
        LOG(message_group::Warning, "CSG normalization resulted in an empty tree");
      }
    }

    if (highlightNodes.size() > 0) {
      LOG("Compiling highlights (%1$i CSG Trees)...", highlightNodes.size());
      this->highlights_products = std::make_shared<CSGProducts>();
      for (auto& highlightNode : highlightNodes) {
        highlightNode = normalizer.normalize(highlightNode);
        this->highlights_products->import(highlightNode);
      }
    }

    if (backgroundNodes.size() > 0) {
      LOG("Compiling background (%1$i CSG Trees)...", backgroundNodes.size());
      this->background_products = std::make_shared<CSGProducts>();
      for (auto& backgroundNode : backgroundNodes) {
        backgroundNode = normalizer.normalize(backgroundNode);
        this->background_products->import(backgroundNode);
      }
    }
    return true;
  }
};
