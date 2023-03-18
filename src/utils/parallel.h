#pragma once

#ifdef ENABLE_TBB
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <thrust/execution_policy.h>
#endif

template <class InputIterator, class OutputIterator, class Operation>
void parallelizable_transform(
  const InputIterator begin1, const InputIterator end1,
  OutputIterator out, 
  const Operation &op)
{
#ifdef ENABLE_TBB
  if (!getenv("OPENSCAD_NO_PARALLEL")) {
    thrust::transform(begin1, end1, out, op);
  }
  else
#endif
  {
    std::transform(begin1, end1, out, op);
  }
}

template <class Container1, class Container2, class OutputIterator, class Operation>
void parallelizable_cross_product_transform(
  const Container1 &cont1,
  const Container2 &cont2,
  OutputIterator out, 
  const Operation &op)
{
#ifdef ENABLE_TBB
  if (!getenv("OPENSCAD_NO_PARALLEL")) {
    struct ReferencePair {
      decltype(*cont1.begin()) first;
      decltype(*cont2.begin()) second;
      ReferencePair(decltype(*cont1.begin()) first, decltype(*cont2.begin()) second) : first(first), second(second) {}
    };
    std::vector<ReferencePair> pairs;
    pairs.reserve(cont1.size() * cont2.size());
    for (const auto &v1 : cont1) {
      for (const auto &v2 : cont2) {
        pairs.emplace_back(v1, v2);
      }
    }
    thrust::transform(pairs.begin(), pairs.end(), out, [&](const auto &pair) {
      return op(pair.first, pair.second);
    });
  }
  else
#endif
  {
    for (const auto &v1 : cont1) {
      for (const auto &v2 : cont2) {
        *(out++) = op(v1, v2);
      }
    }
  }
}