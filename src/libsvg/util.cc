/*
 * The MIT License
 *
 * Copyright (c) 2016-2018, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "libsvg/util.h"

#include <ostream>
#include <boost/spirit/include/qi.hpp>

#include <string>
#include <vector>

// include fusion headers for Ubuntu trusty, everything later seems happy without
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


BOOST_FUSION_ADAPT_STRUCT(libsvg::length_struct, (double, number)(std::string, unit))

namespace libsvg {

namespace qi = boost::spirit::qi;

double
parse_double(const std::string& number)
{
  std::string::const_iterator iter = number.begin(), end = number.end();

  qi::real_parser<double, qi::real_policies<double>> double_parser;

  double d = 0.0;
  const bool result = boost::spirit::qi::parse(iter, end, double_parser, d);

  return result && iter == end ? d : 0;
}

static unit_t
get_unit(const std::string& value)
{
  if (value == "em") {
    return unit_t::EM;
  } else if (value == "ex") {
    return unit_t::EX;
  } else if (value == "px") {
    return unit_t::PX;
  } else if (value == "in") {
    return unit_t::IN;
  } else if (value == "cm") {
    return unit_t::CM;
  } else if (value == "mm") {
    return unit_t::MM;
  } else if (value == "pt") {
    return unit_t::PT;
  } else if (value == "pc") {
    return unit_t::PC;
  } else if (value == "%") {
    return unit_t::PERCENT;
  } else {
    return unit_t::NONE;
  }
}

const length_t
parse_length(const std::string& value)
{
  std::string::const_iterator it = value.begin(), end = value.end();

  qi::rule<std::string::const_iterator, length_struct(), qi::space_type> length;
  qi::rule<std::string::const_iterator, double()> number;
  qi::rule<std::string::const_iterator, std::vector<char>()> unit;

  length = number >> -unit;
  number = qi::double_;
  unit = qi::string("em") | qi::string("ex") | qi::string("px") | qi::string("in") | qi::string("cm") | qi::string("mm") | qi::string("pt") | qi::string("pc") | qi::string("%");

  libsvg::length_struct parsed;
  qi::phrase_parse(it, end, length, qi::space, parsed);

  length_t result{0.0, unit_t::UNDEFINED};
  if ((it != value.begin()) && (it == end)) {
    result.number = parsed.number;
    result.unit = get_unit(parsed.unit);
  }

  return result;
}

const viewbox_t
parse_viewbox(const std::string& value)
{
  std::string::const_iterator it = value.begin(), end = value.end();

  qi::rule<std::string::const_iterator, std::vector<double>(), qi::space_type> viewbox;
  qi::rule<std::string::const_iterator, double()> number;
  qi::rule<std::string::const_iterator> sep;

  viewbox = number >> -sep >> number >> -sep >> number >> -sep >> number;
  number = qi::double_;
  sep = qi::char_(',');

  std::vector<double> parsed;
  qi::phrase_parse(it, end, viewbox, qi::space, parsed);

  viewbox_t result{0.0, 0.0, 0.0, 0.0, false};
  if ((it != value.begin()) && (it == end) && (parsed.size() == 4) && (parsed[2] >= 0.0) && (parsed[3] >= 0.0)) {
    result.x = parsed[0];
    result.y = parsed[1];
    result.width = parsed[2];
    result.height = parsed[3];
    result.is_valid = true;
  }

  return result;
}

const alignment_t
parse_alignment(const std::string& value)
{
  std::string::const_iterator it = value.begin(), end = value.end();

  qi::rule<std::string::const_iterator, std::vector<std::string>(), qi::space_type> alignment;
  qi::rule<std::string::const_iterator, std::vector<char>()> defer;
  qi::rule<std::string::const_iterator, std::vector<char>()> align;
  qi::rule<std::string::const_iterator, std::vector<char>()> meet_or_slice;

  alignment = -qi::as_string[defer] >> qi::as_string[align] >> -qi::as_string[meet_or_slice];
  defer = qi::string("defer");
  align = qi::string("none")
    | qi::string("xMinYMin") | qi::string("xMidYMin") | qi::string("xMaxYMin")
    | qi::string("xMinYMid") | qi::string("xMidYMid") | qi::string("xMaxYMid")
    | qi::string("xMinYMax") | qi::string("xMidYMax") | qi::string("xMaxYMax");
  meet_or_slice = qi::string("meet") | qi::string("slice");

  std::vector<std::string> parsed;
  qi::phrase_parse(it, end, alignment, qi::space, parsed);

  alignment_t result{align_t::MID, align_t::MID, false, true};
  if ((it != value.begin()) && (it == end) && parsed.size() > 0) {
    unsigned int idx = 0;
    if (parsed[0] == "defer") {
      idx++;
      result.defer = true;
    }
    if (parsed.size() > idx) {
      if (parsed[idx] == "none") {
        result.x = align_t::NONE;
        result.y = align_t::NONE;
      } else if (parsed[idx] == "xMinYMin") {
        result.x = align_t::MIN;
        result.y = align_t::MIN;
      } else if (parsed[idx] == "xMidYMin") {
        result.x = align_t::MID;
        result.y = align_t::MIN;
      } else if (parsed[idx] == "xMaxYMin") {
        result.x = align_t::MAX;
        result.y = align_t::MIN;
      } else if (parsed[idx] == "xMinYMid") {
        result.x = align_t::MIN;
        result.y = align_t::MID;
      } else if (parsed[idx] == "xMidYMid") {
        result.x = align_t::MID;
        result.y = align_t::MID;
      } else if (parsed[idx] == "xMaxYMid") {
        result.x = align_t::MAX;
        result.y = align_t::MID;
      } else if (parsed[idx] == "xMinYMax") {
        result.x = align_t::MIN;
        result.y = align_t::MAX;
      } else if (parsed[idx] == "xMidYMax") {
        result.x = align_t::MID;
        result.y = align_t::MAX;
      } else if (parsed[idx] == "xMaxYMax") {
        result.x = align_t::MAX;
        result.y = align_t::MAX;
      }
    }
    idx++;
    if (parsed.size() > idx) {
      result.meet = parsed[idx] == "meet";
    }
  }
  return result;
}

std::ostream& operator<<(std::ostream& stream, const unit_t& unit)
{
  switch (unit) {
  case unit_t::EM:
    stream << "em";
    break;
  case unit_t::EX:
    stream << "ex";
    break;
  case unit_t::PX:
    stream << "px";
    break;
  case unit_t::IN:
    stream << "in";
    break;
  case unit_t::CM:
    stream << "cm";
    break;
  case unit_t::MM:
    stream << "mm";
    break;
  case unit_t::PT:
    stream << "pt";
    break;
  case unit_t::PC:
    stream << "pc";
    break;
  case unit_t::PERCENT:
    stream << "%";
    break;
  case unit_t::NONE:
    break;
  case unit_t::UNDEFINED:
    stream << " (invalid)";
    break;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const length_t& length)
{
  stream << length.number << length.unit;
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const align_t& align)
{
  switch (align) {
  case align_t::MIN:
    stream << "min";
    break;
  case align_t::MID:
    stream << "mid";
    break;
  case align_t::MAX:
    stream << "max";
    break;
  case align_t::NONE:
    stream << "none";
    break;
  case align_t::UNDEFINED:
    stream << "undefined";
    break;
  }
  return stream;
}

} // namespace libsvg
