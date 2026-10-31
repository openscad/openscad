#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

// Wraps the raw int32_t value stored in PolySet::color_indices. A non-negative value is an
// index into PolySet::colors; a negative value is a named sentinel. Today the only sentinel is
// -1 (NoColor), the long-standing "no specific color" meaning.
//
// Trivially copyable and the same size as int32_t, so std::vector<color_index_t> has the same
// layout as std::vector<int32_t> — no change to PolySet's serialization or memory footprint.
class color_index_t
{
public:
  static constexpr int32_t kNoColor = -1;

  constexpr color_index_t() = default;
  // Intentionally implicit: color_indices is populated all over the codebase with plain ints
  // and literals (-1, colors.size(), ...); requiring `color_index_t{...}` at every call site
  // would be ceremony without benefit.
  constexpr color_index_t(int32_t raw) : raw_(raw) {}

  constexpr int32_t raw() const { return raw_; }
  constexpr bool isNoColor() const { return raw_ == kNoColor; }

  // The only way to get an actual index into PolySet::colors: nullopt for every sentinel, so a
  // missing value can't accidentally be used to index the array.
  std::optional<size_t> index() const
  {
    if (raw_ < 0) return std::nullopt;
    return static_cast<size_t>(raw_);
  }

  constexpr bool operator==(const color_index_t& o) const { return raw_ == o.raw_; }
  constexpr bool operator!=(const color_index_t& o) const { return raw_ != o.raw_; }

private:
  int32_t raw_ = kNoColor;
};

static_assert(sizeof(color_index_t) == sizeof(int32_t),
              "color_index_t must stay layout-compatible with int32_t");
