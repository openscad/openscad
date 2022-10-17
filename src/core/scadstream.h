#pragma once

#include <ostream>
#include <sstream>
#include <string>
#include <iterator>
#include <double-conversion/double-conversion.h>

/* Define values for double-conversion library. */
#define DC_BUFFER_SIZE 128
#define DC_FLAGS (double_conversion::DoubleToStringConverter::UNIQUE_ZERO)
#define DC_INF "inf"
#define DC_NAN "nan"
#define DC_EXP 'e'
#define DC_DECIMAL_LOW_EXP -5
#define DC_DECIMAL_HIGH_EXP 6
#define DC_MAX_LEADING_ZEROES 5
#define DC_MAX_TRAILING_ZEROES 0

namespace scad {

class ostringstream
{
public:
  ostringstream() :
      oss(),
      builder(buffer, DC_BUFFER_SIZE),
      dc(DC_FLAGS, DC_INF, DC_NAN, DC_EXP,
          DC_DECIMAL_LOW_EXP, DC_DECIMAL_HIGH_EXP,
          DC_MAX_LEADING_ZEROES, DC_MAX_TRAILING_ZEROES)
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
  char buffer[DC_BUFFER_SIZE];
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