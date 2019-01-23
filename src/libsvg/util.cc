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
#include <boost/spirit/include/qi.hpp>

// include fusion headers for Ubuntu trusty, everything later seems happy without
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "util.h"

BOOST_FUSION_ADAPT_STRUCT(libsvg::length_struct, (double, number) (std::string, unit))

namespace libsvg {

namespace qi = boost::spirit::qi;

double
parse_double(const std::string& number)
{
   std::string::const_iterator iter = number.begin(), end = number.end();
   
   qi::real_parser<double, qi::real_policies<double> > double_parser;
   
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
	} else {
		return unit_t::NONE;
	}
}

const length_t
parse_length(const std::string& value)
{
	std::string::const_iterator it = value.begin(), end = value.end();

	qi::rule<std::string::const_iterator, length_struct> length;
	qi::rule<std::string::const_iterator, std::vector<char>> unit;

	length = qi::double_ >> -unit;
	unit = qi::string("em") | qi::string("ex") | qi::string("px") | qi::string("in") | qi::string("cm") | qi::string("mm") | qi::string("pt") | qi::string("pc");

	libsvg::length_struct parsed;
	qi::phrase_parse(it, end, length, qi::space, parsed);

	length_t result;
	if (it == end) {
		result.number = parsed.number;
		result.unit = get_unit(parsed.unit);
	}

	return result;
}

}
