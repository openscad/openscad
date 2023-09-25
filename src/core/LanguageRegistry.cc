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

#include "LanguageRegistry.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

LanguageRegistry *LanguageRegistry::inst = nullptr;

void LanguageRegistry::setDefaultRuntime(LanguageRuntime* runtime)
{
  defaultRuntime = runtime;
}

LanguageRuntime* LanguageRegistry::getDefaultRuntime() { return defaultRuntime; }

LanguageRuntime* LanguageRegistry::getRuntime(std::string id) { 
  auto it = this->entries.find(id); 
  return it != this->entries.end() ? it->second.runtime : nullptr;
}

LanguageRuntime* LanguageRegistry::getRuntimeForFileName(std::string filename) { 
  std::string extension = boost::filesystem::path(filename).extension().string();
  if(extension.empty()) return getDefaultRuntime();
  for (const auto& [k, e] : this->entries)
  {
    if (boost::iequals(e.runtime->getFileExtension(), extension)) {
      if (e.active) 
        return e.runtime;
      else
        return getDefaultRuntime();
    }
  }
  return getDefaultRuntime();
}

LanguageRuntime* LanguageRegistry::getRuntimeForFileSuffix(std::string suffix) { 
  if(suffix.empty()) return getDefaultRuntime();
  for (const auto& [k, e] : this->entries)
  {
    if (boost::iequals(e.runtime->getFileSuffix(), suffix)) {
      if (e.active) 
        return e.runtime;
      else
        return getDefaultRuntime();
    }
  }
  return getDefaultRuntime();
}


bool LanguageRegistry::isActive(std::string id) {
  auto it = this->entries.find(id); 
  return it != this->entries.end() ? it->second.active : false;
}

std::vector<LanguageRuntime*> LanguageRegistry::activeRuntimes() {
  std::vector<LanguageRuntime*> active_runtimes = {};
  for (const auto& [k, e] : this->entries)
    if (e.active) {
     active_runtimes.push_back(e.runtime);
    }
  return active_runtimes;
}

std::vector<std::string> LanguageRegistry::fileExtensions() {
  std::vector<std::string> extensions;
  for (const auto& [k, e] : this->entries)
    if (e.active) {
     extensions.push_back(e.runtime->getFileExtension());
    }
  return extensions;
}

std::vector<std::string> LanguageRegistry::fileSuffixes() {
  std::vector<std::string> extensions;
  for (const auto& [k, e] : this->entries)
    if (e.active) {
     extensions.push_back(e.runtime->getFileSuffix());
    }
  return extensions;
}

std::vector<std::string> LanguageRegistry::fileFilters() {
  std::vector<std::string> filters;
  for (const auto& [k, e] : this->entries)
    if (e.active) {
     filters.push_back(e.runtime->getFileFilter());
    }
  return filters;
}

std::string LanguageRegistry::formatFileFilters(std::string seperator) {
  auto list = this->fileFilters();
  std::string str;
  str = boost::algorithm::join(list, seperator); //";;"
  return str;
}