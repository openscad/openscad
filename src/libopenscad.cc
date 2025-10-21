/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  021111307  USA
 *
 */

#include "openscad.h"
#include "libopenscad.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "utils/printutils.h"


// Extern "C" function for initializing and running OpenSCAD from a Native C API.
OPENSCAD_API int lib_openscad(int argc, const char** argv)
{
  try {
    std::vector<char*> arg_ptrs;
    for (int i = 0; i < argc; ++i) {
      arg_ptrs.push_back(const_cast<char*>(argv[i]));
    }

    // Store the original current working directory.
    const auto original_path = fs::current_path();

    // Execute the main OpenSCAD functionality with the prepared arguments.
    const int result = openscad_main(argc, arg_ptrs.data(), true);

    // Restore the original working directory after execution.
    fs::current_path(original_path);

    // Return the result of the OpenSCAD execution.
    return result;
  } catch (const std::exception& e) {
    // Log any exception that occurs during command line execution and return 1 to indicate failure.
    LOG("Command line execution failed: %1$s", e.what());
    return 1;
  }
}