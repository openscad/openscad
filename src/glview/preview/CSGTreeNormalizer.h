#pragma once

#include "memory.h"

class CSGTreeNormalizer
{
public:
  CSGTreeNormalizer(size_t limit) : limit(limit) {}

  shared_ptr<class CSGNode> normalize(const shared_ptr<CSGNode>& term);

private:
  shared_ptr<CSGNode> normalizePass(shared_ptr<CSGNode> term);
  bool match_and_replace(shared_ptr<class CSGNode>& term);
  shared_ptr<CSGNode> collapse_null_terms(const shared_ptr<CSGNode>& term);
  shared_ptr<CSGNode> cleanup_term(shared_ptr<CSGNode>& t);
  [[nodiscard]] unsigned int count(const shared_ptr<CSGNode>& term) const;

  bool aborted{false};
  size_t limit;
  size_t nodecount{0};
  shared_ptr<class CSGNode> rootnode;
};
