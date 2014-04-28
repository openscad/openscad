#pragma once

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// FIXME: boostfs_relative_path() has been replaced by 
// boostfs_uncomplete(), but kept around for now.
fs::path boostfs_relative_path(const fs::path &path, const fs::path &relative_to);
fs::path boostfs_normalize(const fs::path &path);
fs::path boostfs_uncomplete(fs::path const p, fs::path const base);
