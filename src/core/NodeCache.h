#pragma once

#include <utility>
#include <string>
#include <unordered_map>
#include <cassert>
#include <cstddef>

#include "core/node.h"
#include "utils/printutils.h"

/*!
   Caches string values per node based on the node.index().
   The node index guaranteed to be unique per node tree since the index is reset
   every time a new tree is generated.
 */

class NodeCache
{
public:
  NodeCache() = default;

  bool contains(const AbstractNode& node) const
  {
    auto result = this->cache.find(node.index());
    return result != this->cache.end() && result->second.second >= 0L &&
           (long)this->rootString.size() >= result->second.second;
  }

  std::string operator[](const AbstractNode& node) const
  {
    // throws std::out_of_range on miss
    auto indexpair = this->cache.at(node.index());
    return rootString.substr(indexpair.first, indexpair.second - indexpair.first);
  }

  void insertStart(const size_t nodeidx, const long startindex)
  {
#ifdef ENABLE_PYTHON
    if (true) {
      if (this->cache.count(nodeidx) ==
          0)  // with python it can happen that nodes get dumped several times,
              // but its understood that the dump will always be identical
        this->cache.emplace(nodeidx, std::make_pair(startindex, -1L));
    } else
#endif
    {
      assert(this->cache.count(nodeidx) == 0 && "start index inserted twice");
      this->cache.emplace(nodeidx, std::make_pair(startindex, -1L));
    }
  }

  void insertEnd(const size_t nodeidx, const long endindex)
  {
    // throws std::out_of_range on miss
    std::pair<long int, long int> indexpair;
    try {
      indexpair = this->cache.at(nodeidx);
    } catch (std::exception& ex) {
      return;
    }
#ifdef ENABLE_PYTHON
    if (true) {
      if (indexpair.second == -1L) this->cache[nodeidx] = std::make_pair(indexpair.first, endindex);
    } else
#endif
    {
      assert(indexpair.second == -1L && "end index inserted twice");
      this->cache[nodeidx] = std::make_pair(indexpair.first, endindex);
    }
#ifdef DEBUG
    PRINTDB("NodeCache insert {%i,[%d:%d]}", nodeidx % indexpair.first % endindex);
#endif
  }

  void setRootString(const std::string& rootString) { this->rootString = rootString; }

  void clear()
  {
    this->cache.clear();
    this->rootString = "";
  }

private:
  std::unordered_map<size_t, std::pair<long, long>> cache;
  std::string rootString;
};
