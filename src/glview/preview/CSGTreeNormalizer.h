#pragma once

#include <cstddef>
#include <memory>

class CSGTreeNormalizer
{
public:
  CSGTreeNormalizer(size_t limit) : limit(limit) {}

  std::shared_ptr<class CSGNode> normalize(const std::shared_ptr<CSGNode>& term);

private:
  std::shared_ptr<CSGNode> normalizePass(std::shared_ptr<CSGNode> term);
  bool match_and_replace(std::shared_ptr<class CSGNode>& term);
  std::shared_ptr<CSGNode> collapse_null_terms(const std::shared_ptr<CSGNode>& term);
  std::shared_ptr<CSGNode> cleanup_term(std::shared_ptr<CSGNode>& t);
  [[nodiscard]] unsigned int count(const std::shared_ptr<CSGNode>& term) const;

  bool aborted{false};
  size_t limit;
  size_t nodecount{0};
  std::shared_ptr<class CSGNode> rootnode;
};
