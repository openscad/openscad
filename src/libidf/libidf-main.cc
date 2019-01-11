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

#include <iostream>
#include <boost/program_options.hpp>

#include "libidf.h"

namespace po = boost::program_options;

class section_visitor : public boost::static_visitor<void> {
public:

	void operator()(const libidf::outline_struct &section) const
	{
		std::cout << "outline" << std::endl;
	}

	void operator()(const libidf::other_outline_struct &section) const
	{
		std::cout << "other_outline" << std::endl;
	}

	void operator()(const libidf::drilled_holes_struct &section) const
	{
		std::cout << "drilled_holes" << std::endl;
	}

	void operator()(const libidf::route_outline_struct &section) const
	{
		std::cout << "route_outline" << std::endl;
	}

	void operator()(const libidf::place_outline_struct &section) const
	{
		std::cout << "place_outline" << std::endl;
	}

	void operator()(const libidf::route_keepout_struct &section) const
	{
		std::cout << "route_keepout" << std::endl;
	}

	void operator()(const libidf::place_keepout_struct &section) const
	{
		std::cout << "place_keepout" << std::endl;
	}

	void operator()(const libidf::via_keepout_struct &section) const
	{
		std::cout << "via_keepout" << std::endl;
	}

	void operator()(const libidf::place_region_struct &section) const
	{
		std::cout << "place_region" << std::endl;
	}

	void operator()(const libidf::notes_struct &section) const
	{
		std::cout << "notes_struct" << std::endl;
		for (const auto &note : section) {
			std::cout << "  " << note.x << " / " << note.y << " / " << note.value << std::endl;
		}
	}

	void operator()(const libidf::placement_struct &section) const
	{
		std::cout << "placement_struct" << std::endl;
		for (const auto &comp : section) {
			std::cout << "  " << comp.package_name << " / " << comp.part_number << " / " << comp.placement_status << std::endl;
		}
	}
};

int main(int argc, char **argv)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("file", po::value<std::string>()->required(), "file to import")
		;
	po::positional_options_description p;
	p.add("file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	po::notify(vm);    
	
	const auto file = vm["file"].as<std::string>();
	const libidf::idf_struct idf = libidf::read_file(file);

	std::cout << idf.header.board_name << " " << idf.header.date << std::endl;
	for (const auto &section : idf.sections) {
		boost::apply_visitor(section_visitor(), section);		
	}

	return 0;
}
