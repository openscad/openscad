#pragma once

#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

extern int parser_error_pos;

/**
 * Initialize application an library path.
 * 
 * @param applicationpath path of the application binary, this is usually
 * derived from the Qt application object. If Qt is disabled, argv[0] is used.
 */
void parser_init(const std::string &applicationpath);

/**
 * Return a path to specific resources relative to the application binary.
 * This is used to find resources bundled with the application, e.g. the
 * translation files for the gettext library.
 * 
 * @param folder subfolder for the resources (e.g. "po").
 * @return the resource path.
 */
fs::path get_resource_dir(const std::string &resource_folder);

fs::path search_libs(const fs::path &localpath);
fs::path find_valid_path(const fs::path &sourcepath, 
                         const fs::path &localpath, 
                         const std::vector<std::string> *openfilenames = NULL);
