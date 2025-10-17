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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/optional/optional.hpp>
#ifdef ENABLE_CGAL
#include <CGAL/assertions.h>
#include <CGAL/assertions_behaviour.h>
#endif

#include "core/Builtins.h"
#include "openscad_mimalloc.h"
#include "platform/PlatformUtils.h"
#include "utils/printutils.h"

static bool g_initialized = false;

int openscad_init()
{
  // Check if the library has already been initialized.
  if (g_initialized) {
    // If it's already initialized, return 0 indicating success.
    return 0;
  }
  try {
    // Conditional compilation: Initialize mimalloc if both ENABLE_CGAL and USE_MIMALLOC are defined.
#if defined(ENABLE_CGAL) && defined(USE_MIMALLOC)
    init_mimalloc();
#endif

    // Determine the application path differently based on whether the code is running under Emscripten or not.
#ifndef __EMSCRIPTEN__
    // For non-Emscripten builds, get the parent directory of the program location.
    const auto applicationPath = weakly_canonical(boost::dll::program_location()).parent_path().generic_string();
#else
    // For Emscripten builds, use the current working directory as the application path.
    const auto applicationPath = boost::dll::fs::current_path();
#endif

    // Register the determined application path with the PlatformUtils.
    PlatformUtils::registerApplicationPath(applicationPath);

    // Set CGAL error and warning behaviors to throw exceptions if ENABLE_CGAL is defined.
#ifdef ENABLE_CGAL
    CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
    CGAL::set_warning_behaviour(CGAL::THROW_EXCEPTION);
#endif

    // Initialize the built-in functions and set the global initialization flag.
    Builtins::instance()->initialize();
    g_initialized = true;

    // Return 0 to indicate successful initialization.
    return 0;
  } catch (const std::exception& e) {
    // Log any exception that occurs during initialization and return -1 to indicate failure.
    LOG("Initialization failed: %1$s", e.what());
    return -1;
  }
}

// Extern "C" function for initializing and running OpenSCAD from a Native C API.
OPENSCAD_API int lib_openscad(int argc, const char** argv)
{
  // Ensure the library is initialized before proceeding.
  if (!g_initialized) {
    // If initialization fails, return -1.
    if (openscad_init() != 0) {
      return -1;
    }
  }

  try {
    // Prepare vectors to store command line arguments as strings and pointers to these strings.
    std::vector<std::string> args;
    std::vector<char*> arg_ptrs;

    // Populate the args vector with the provided command line arguments.
    for (int i = 0; i < argc; ++i) {
      std::cout << argv[i] << std::endl; // Print each argument for debugging purposes.
      args.push_back(argv[i]);
    }

    // Convert the string arguments into character pointers and store them in arg_ptrs.
    for (auto& arg : args) {
      arg_ptrs.push_back(&arg[0]);
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
    // Log any exception that occurs during command line execution and return -1 to indicate failure.
    LOG("Command line execution failed: %1$s", e.what());
    return -1;
  }
}