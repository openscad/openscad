#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>

#if ENABLE_TBB
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#endif

template <class InputIterator, class OutputIterator, class Operation>
void parallelizable_transform(const InputIterator begin1,
                              const InputIterator end1, OutputIterator out,
                              const Operation &op) {
#if ENABLE_TBB
  if (!getenv("OPENSCAD_NO_PARALLEL")) {
    tbb::parallel_for(tbb::blocked_range(begin1, end1), [&](auto range) {
      size_t start_index = std::distance(begin1, range.begin());
      for (auto iter = range.begin(); iter != range.end(); iter++)
        out[start_index++] = op(*iter);
    });
    return;
  }
#endif
  std::transform(begin1, end1, out, op);
}

template <class Container1, class Container2, class OutputIterator,
          class Operation>
void parallelizable_cross_product_transform(const Container1 &cont1,
                                            const Container2 &cont2,
                                            OutputIterator out,
                                            const Operation &op) {
#if ENABLE_TBB
  if (!getenv("OPENSCAD_NO_PARALLEL")) {
    struct ReferencePair {
      decltype(*cont1.begin()) first;
      decltype(*cont2.begin()) second;
      ReferencePair(decltype(*cont1.begin()) first,
                    decltype(*cont2.begin()) second)
          : first(first), second(second) {}
    };
    std::vector<ReferencePair> pairs;
    pairs.reserve(cont1.size() * cont2.size());
    for (const auto &v1 : cont1) {
      for (const auto &v2 : cont2) {
        pairs.emplace_back(v1, v2);
      }
    }
    parallelizable_transform(
        pairs.begin(), pairs.end(), out,
        [&](const auto &pair) { return op(pair.first, pair.second); });
    return;
  }
#endif
  for (const auto &v1 : cont1) {
    for (const auto &v2 : cont2) {
      *(out++) = op(v1, v2);
    }
  }
}
