#pragma once
#include <algorithm>

template <class InputIterator, class OutputIterator, class Operation>
void parallelizable_transform(
  const InputIterator begin1, const InputIterator end1,
  OutputIterator out, 
  const Operation &op)
{
  std::transform(begin1, end1, out, op);
}

template <class Container1, class Container2, class OutputIterator, class Operation>
void parallelizable_cross_product_transform(
  const Container1 &cont1,
  const Container2 &cont2,
  OutputIterator out, 
  const Operation &op)
{
  for (const auto &v1 : cont1) {
    for (const auto &v2 : cont2) {
      *(out++) = op(v1, v2);
    }
  }
}
