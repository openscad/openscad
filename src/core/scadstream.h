#pragma once

#include <ostream>
#include <sstream>
#include <string>
#include <iterator>
#include <double-conversion/double-conversion.h>

namespace scad {

class ostringstream
{
public:
  using dc_flags_t = double_conversion::DoubleToStringConverter::Flags;
  static constexpr auto dc_flags = dc_flags_t::UNIQUE_ZERO;
  static constexpr size_t dc_buffer_size = 128;
  static constexpr const char * dc_inf = "inf";
  static constexpr const char * dc_nan = "nan";
  static constexpr char dc_exp = 'e';
  static constexpr int dc_decimal_low_exp = -5;
  static constexpr int dc_decimal_high_exp = 6;
  static constexpr int dc_max_leading_zeros = 5;
  static constexpr int dc_max_trailing_zeros = 0;

  ostringstream() :
      oss(),
      builder(buffer, dc_buffer_size),
      dc(dc_flags, dc_inf, dc_nan, dc_exp,
          dc_decimal_low_exp, dc_decimal_high_exp,
          dc_max_leading_zeros, dc_max_trailing_zeros)
  {}

  using CharT = char;
  using Traits = std::char_traits<CharT>;
  using Allocator = std::allocator<CharT>;

  using char_type      = CharT;
  using int_type       = typename Traits::int_type;
  using pos_type       = typename Traits::pos_type;
  using off_type       = typename Traits::off_type;
  using traits_type    = Traits;
  using allocator_type = Allocator;

  std::string str() const { return oss.str(); }
  void str(const std::string& s) { oss.str(s); }
  void clear( std::ios_base::iostate state = std::ios_base::goodbit ) { oss.clear(state); }
  pos_type tellp() { return oss.tellp(); }
  std::ostream_iterator<std::string> get_os_iterator() { return oss; }

  template <typename T>
  friend ostringstream& operator<<(ostringstream&, const T&);

  // Additional overload to handle ostream specific io manipulators
  friend scad::ostringstream& operator<<(scad::ostringstream&, std::ostream& (*)(std::ostream&));

  // Accessor function to get a reference to the ostream
  //std::ostringstream& get_ostringstream() { return *this; }
protected:
  std::ostringstream oss;
  char buffer[dc_buffer_size];
  double_conversion::StringBuilder builder;
  double_conversion::DoubleToStringConverter dc;
};

template <typename T>
inline scad::ostringstream& operator<<(scad::ostringstream& out, const T& value)
{
    out.oss << value;
    return out;
}

//  overload for double
template <>
inline scad::ostringstream& operator<<(scad::ostringstream& out, const double& value)
{
  out.builder.Reset();
  out.dc.ToShortest(value, &(out.builder));
  out.oss << out.builder.Finalize();
  return out;
}

//  overload for std::ostringstream specific io manipulators
inline ostringstream& operator<<(ostringstream& out, std::ostream& (*func)(std::ostream&))
{
  out.oss << func;
  return out;
}

} // namespace scad