#pragma once

class RangeType
{
private:
  double begin_val;
  double step_val;
  double end_val;
  enum class iter_state { RANGE_BEGIN, RANGE_RUNNING, RANGE_END };

public:
  static constexpr uint32_t MAX_RANGE_STEPS = 10000;
  static const RangeType EMPTY;

  class iterator
  {
public:
    // iterator_traits required types:
    using iterator_category = std::forward_iterator_tag;
    using value_type = double;
    using difference_type = void; // type used by operator-(iterator), not implemented for forward iterator
    using reference = value_type; // type used by operator*(), not actually a reference
    using pointer = void;     // type used by operator->(), not implemented
    iterator(const RangeType& range, iter_state type);
    iterator& operator++();
    reference operator*();
    bool operator==(const iterator& other) const;
    bool operator!=(const iterator& other) const;
private:
    const RangeType& range;
    double val;
    iter_state state;
    const uint32_t num_values;
    uint32_t i_step;
    void update_state();
  };

  RangeType(const RangeType&) = delete;       // never copy, move instead
  RangeType& operator=(const RangeType&) = delete; // never copy, move instead
  RangeType(RangeType&&) = default;
  RangeType& operator=(RangeType&&) = default;
  ~RangeType() = default;

  explicit RangeType(double begin, double end)
    : begin_val(begin), step_val(1.0), end_val(end) {}

  explicit RangeType(double begin, double step, double end)
    : begin_val(begin), step_val(step), end_val(end) {}

  bool operator==(const RangeType& other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return n2 == 0;
    if (n2 == 0) return false;
    return this == &other ||
           (this->begin_val == other.begin_val &&
            this->step_val == other.step_val &&
            n1 == n2);
  }

  bool operator!=(const RangeType& other) const {
    return !(*this == other);
  }

  bool operator<(const RangeType& other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return 0 < n2;
    if (n2 == 0) return false;
    return this->begin_val < other.begin_val ||
           (this->begin_val == other.begin_val &&
            (this->step_val < other.step_val || (this->step_val == other.step_val && n1 < n2))
           );
  }

  bool operator<=(const RangeType& other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n1 == 0) return true; // (0 <= n2) is always true
    if (n2 == 0) return false;
    return this->begin_val < other.begin_val ||
           (this->begin_val == other.begin_val &&
            (this->step_val < other.step_val || (this->step_val == other.step_val && n1 <= n2))
           );
  }

  bool operator>(const RangeType& other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n2 == 0) return n1 > 0;
    if (n1 == 0) return false;
    return this->begin_val > other.begin_val ||
           (this->begin_val == other.begin_val &&
            (this->step_val > other.step_val || (this->step_val == other.step_val && n1 > n2))
           );
  }

  bool operator>=(const RangeType& other) const {
    auto n1 = this->numValues();
    auto n2 = other.numValues();
    if (n2 == 0) return true; // (n1 >= 0) is always true
    if (n1 == 0) return false;
    return this->begin_val > other.begin_val ||
           (this->begin_val == other.begin_val &&
            (this->step_val > other.step_val || (this->step_val == other.step_val && n1 >= n2))
           );
  }

  [[nodiscard]] double begin_value() const { return begin_val; }
  [[nodiscard]] double step_value() const { return step_val; }
  [[nodiscard]] double end_value() const { return end_val; }

  [[nodiscard]] iterator begin() const { return {*this, iter_state::RANGE_BEGIN}; }
  [[nodiscard]] iterator end() const { return {*this, iter_state::RANGE_END}; }

  /// return number of values, max uint32_t value if step is 0 or range is infinite
  [[nodiscard]] uint32_t numValues() const;
};
std::ostream& operator<<(std::ostream& stream, const RangeType& r);
