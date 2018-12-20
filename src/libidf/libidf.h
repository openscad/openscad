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

#pragma once

#include <string>
#include <vector>
#include <boost/variant.hpp>

namespace libidf {

struct header_struct
{
	std::string file_type;
	std::string version;
	std::string system;
	std::string date;
	std::string revision;
	std::string board_name;
	std::string units;
};

struct loop_struct
{
	int label;
	double x;
	double y;
	double angle;
};

struct outline_struct
{
	std::string type;
	std::string owner;
	double thickness;
	std::vector<loop_struct> loop_list;
};

struct other_outline_struct
{
	std::string owner;
	std::string identifier;
	std::string board_side;
	double thickness;
	std::vector<loop_struct> loop_list;
};

struct route_struct
{
	std::string owner;
	std::string layer;
	std::vector<loop_struct> loop_list;
};

struct route_outline_struct : route_struct { };

struct place_struct
{
	std::string owner;
        std::string board_side;
        double height;
	std::vector<loop_struct> loop_list;
};

struct place_outline_struct : place_struct { };

struct route_keepout_struct : route_struct { };

struct place_keepout_struct : place_struct { };

struct via_keepout_struct
{
	std::string owner;
	std::vector<loop_struct> loop_list;
};

struct place_region_struct
{
	std::string owner;
        std::string board_side;
        std::string component_group_name;
	std::vector<loop_struct> loop_list;
};

struct hole_struct
{
	double diameter;
	double x;
	double y;
	std::string plating_style;
	std::string associated_part;
	std::string hole_type;
	std::string hole_owner;
};

using drilled_holes_struct = std::vector<hole_struct>;

struct note_struct
{
        double x;
        double y;
        double height;
        double length;
        std::string value;
};

using notes_struct = std::vector<note_struct>;

struct component_struct
{
    std::string package_name;
    std::string part_number;
    std::string refdes;
    double x;
    double y;
    double mounting_offset;
    double rotation_angle;
    std::string board_side;
    std::string placement_status;
};

using placement_struct = std::vector<component_struct>;

using section_struct = boost::variant<
        outline_struct,
        other_outline_struct,
        route_outline_struct,
        place_outline_struct,
        route_keepout_struct,
        place_keepout_struct,
        via_keepout_struct,
        place_region_struct,
        drilled_holes_struct,
        notes_struct,
        placement_struct>;

struct idf_struct
{
	header_struct header;
	std::vector<section_struct> sections;
};

idf_struct read_file(const std::string &filename);

}