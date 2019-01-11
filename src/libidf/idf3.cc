/*
 * The MIT License
 *
 * Copyright (c) 2018, Torsten Paul <torsten.paul@gmx.de>
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

#include <fstream>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_stream.hpp>

#include "libidf.h"

namespace qi = boost::spirit::qi;

BOOST_FUSION_ADAPT_STRUCT(libidf::header_struct, (std::string, file_type) (std::string, version) (std::string, system) (std::string, date) (std::string, revision) (std::string, board_name) (std::string, units))
BOOST_FUSION_ADAPT_STRUCT(libidf::loop_struct, (int, label) (double, x) (double, y) (double, angle))
BOOST_FUSION_ADAPT_STRUCT(libidf::outline_struct, (std::string, type) (std::string, owner) (double, thickness) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::other_outline_struct, (std::string, owner) (std::string, identifier) (double, thickness) (std::string, board_side) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::route_outline_struct, (std::string, owner) (std::string, layer) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::place_outline_struct, (std::string, owner) (std::string, board_side) (double, height) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::route_keepout_struct, (std::string, owner) (std::string, layer) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::place_keepout_struct, (std::string, owner) (std::string, board_side) (double, height) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::via_keepout_struct, (std::string, owner) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::place_region_struct, (std::string, owner) (std::string, board_side) (std::string, component_group_name) (std::vector<libidf::loop_struct>, loop_list))
BOOST_FUSION_ADAPT_STRUCT(libidf::note_struct, (double, x) (double, y) (double, height) (double, length) (std::string, value))
BOOST_FUSION_ADAPT_STRUCT(libidf::component_struct, (std::string, package_name) (std::string, part_number) (std::string, refdes) (double, x) (double, y) (double, mounting_offset) (double, rotation_angle) (std::string, board_side) (std::string, placement_status))
BOOST_FUSION_ADAPT_STRUCT(libidf::hole_struct, (double, diameter) (double, x) (double, y) (std::string, plating_style) (std::string, associated_part) (std::string, hole_type) (std::string, hole_owner))
BOOST_FUSION_ADAPT_STRUCT(libidf::idf_struct, (libidf::header_struct, header) (std::vector<libidf::section_struct>, sections))

namespace libidf {

idf_struct read_file(const std::string &filename) {
	using I = boost::spirit::istream_iterator;
	using rule = qi::rule<I>;
	using srule = qi::rule<I, std::vector<char>>;

	idf_struct result;
	qi::rule<I, idf_struct> idf;
	qi::rule<I, header_struct> header;
	qi::rule<I, std::vector<section_struct>> section_list;
	qi::rule<I, section_struct> section;
	qi::rule<I, outline_struct> outline;
	qi::rule<I, other_outline_struct> other_outline;
	qi::rule<I, route_outline_struct> route_outline;
	qi::rule<I, place_outline_struct> place_outline;
	qi::rule<I, route_keepout_struct> route_keepout;
	qi::rule<I, place_keepout_struct> place_keepout;
	qi::rule<I, via_keepout_struct> via_keepout;
	qi::rule<I, place_region_struct> place_region;
	qi::rule<I, drilled_holes_struct> drilled_holes;
	qi::rule<I, notes_struct> notes;
	qi::rule<I, placement_struct> placement;
	qi::rule<I, loop_struct> loop;
	qi::rule<I, std::vector<loop_struct>> loop_list;
	qi::rule<I, hole_struct> holes;
	qi::rule<I, std::vector<hole_struct>> holes_list;
	qi::rule<I, note_struct> note;
	qi::rule<I, std::vector<note_struct>> note_list;
	qi::rule<I, component_struct> component;
	qi::rule<I, std::vector<component_struct>> component_list;

	srule file_type, outline_type, string, quoted_string, date, unit, owner, version, revision, plating, board_side, board_side_both, layer, placement_status;
	rule comment_or_nl, sp, nl, outline_end;

	idf = header >> +comment_or_nl >> -section_list >> +comment_or_nl;
	header = ".HEADER" >> nl >> file_type >> sp >> version >> sp >> string >> sp >> date >> sp >> revision >> nl >> string >> sp >> unit >> nl >> ".END_HEADER";

	section_list = section % +comment_or_nl;
	section = outline | other_outline | route_outline | place_outline | route_keepout | place_keepout | via_keepout | place_region | drilled_holes | notes | placement;

	outline = outline_type  >> sp >> owner >> nl >> qi::double_ >> nl >> loop_list >> nl >> outline_end;
	other_outline = ".OTHER_OUTLINE" >> sp >> owner >> nl >> -sp >> string >> sp >> qi::double_ >> sp >> board_side >> nl >> loop_list >> nl >> ".END_OTHER_OUTLINE";
	route_outline = ".ROUTE_OUTLINE" >> sp >> owner >> nl >> -sp >> layer >> nl >> loop_list >> nl >> ".END_ROUTE_OUTLINE";
	place_outline = ".PLACE_OUTLINE" >> sp >> owner >> nl >> -sp >> board_side_both >> sp >> qi::double_ >> nl >> loop_list >> nl >> ".END_PLACE_OUTLINE";
	route_keepout = ".ROUTE_KEEPOUT" >> sp >> owner >> nl >> -sp >> layer >> nl >> loop_list >> nl >> ".END_ROUTE_KEEPOUT";
	place_keepout = ".PLACE_KEEPOUT" >> sp >> owner >> nl >> -sp >> board_side_both >> sp >> qi::double_ >> nl >> loop_list >> nl >> ".END_PLACE_KEEPOUT";
	via_keepout = ".VIA_KEEPOUT" >> sp >> owner >> loop_list >> nl >> ".END_VIA_KEEPOUT";
	place_region = ".PLACE_REGION" >> sp >> owner >> nl >> -sp >> board_side_both >> sp >> string >> nl >> loop_list >> nl >> ".END_PLACE_REGION";
	drilled_holes = ".DRILLED_HOLES" >> nl >> holes_list >> nl >> ".END_DRILLED_HOLES";
	notes = ".NOTES" >> nl >> note_list >> nl >> ".END_NOTES";
	placement = ".PLACEMENT" >> nl >> component_list >> nl >> ".END_PLACEMENT";

	loop_list = loop % nl;
	loop = -sp >> qi::int_ >> sp >>qi::double_ >> sp >> qi::double_ >> sp >> qi::double_;

	holes_list = holes % nl;
	holes = -sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> plating >> sp >> string >> sp >> string >> sp >> owner;

	note_list = note % nl;
	note = sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> string;
	
	component_list = component % nl;
	component = sp >> string >> sp >> string >> sp >> string >> nl >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> qi::double_ >> sp >> board_side >> sp >> placement_status;

	file_type = qi::string("BOARD_FILE") | qi::string("PANEL_FILE");
	outline_type = qi::string(".BOARD_OUTLINE") | qi::string(".PANEL_OUTLINE");
	outline_end = qi::lit(".END_BOARD_OUTLINE") | qi::lit(".END_PANEL_OUTLINE");
	unit = qi::string("MM") | qi::string("THOU");
	owner = qi::string("MCAD") | qi::string("ECAD") | qi::string("UNOWNED");
	plating = qi::string("PTH") | qi::string("NPTH");
	board_side = qi::string("TOP") | qi::string("BOTTOM");
	board_side_both = qi::string("TOP") | qi::string("BOTTOM") | qi::string("BOTH");
	layer = qi::string("TOP") | qi::string("BOTTOM") | qi::string("BOTH") | qi::string("INNER") | qi::string("ALL");
	placement_status = qi::string("MCAD") | qi::string("ECAD") | qi::string("PLACED") | qi::string("UNPLACED");
	
	string = quoted_string | +(qi::char_ - qi::space);
	quoted_string = '"' >> *(qi::char_ - '"') >> '"';
	date = +qi::char_("0123456789/.:");
	version = +qi::char_("0-9") >> qi::char_('.') >> +qi::char_("0-9");
	revision = +qi::char_("0-9");
	comment_or_nl = qi::skip[qi::char_('#') >> *(qi::char_ - nl) >> nl | nl];
	nl = qi::skip[+qi::char_("\r\n")];
	sp = qi::skip[+qi::space];

	std::ifstream file(filename, std::ifstream::binary);
	if (file.is_open()) {
		file.unsetf(std::ios::skipws);
		boost::spirit::istream_iterator it{file};
		boost::spirit::istream_iterator end;
		qi::phrase_parse(it, end, idf, qi::space, result);
		file.close();
	}
	return result;
}

}