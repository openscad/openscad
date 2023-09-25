#pragma once

#include "LanguageRuntime.h"
#include "SCADLanguageRuntime.h"
#include "PythonLanguageRuntime.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

/*!
   Language Registry which manages all active language support runtimes; 
 */
class LanguageRegistry
{
public:
   static LanguageRegistry *instance() { 
      if (!inst) {
       inst = new LanguageRegistry; 
       LanguageRuntime* runtime = new SCADLanguageRuntime();
       struct runtime_entry entry = {
         runtime,
         true
       };
       inst->entries.insert({runtime->getId(), entry});
       LanguageRuntime* py_runtime = new PythonLanguageRuntime();
       struct runtime_entry py_entry = {
         py_runtime,
         true
       };
       inst->entries.insert({py_runtime->getId(), py_entry});
      }
      return inst; 
   }

   bool registerRuntime(std::string id, LanguageRuntime* runtime);
   LanguageRuntime* getRuntime(std::string id);
   LanguageRuntime* getRuntimeForFileName(std::string filename);
   LanguageRuntime* getRuntimeForFileSuffix(std::string filename);
   bool isActive(std::string id);
   std::vector<LanguageRuntime*> activeRuntimes();
   std::vector<std::string> fileExtensions();
   std::vector<std::string> fileSuffixes();
   std::vector<std::string> fileFilters();
   std::string formatFileFilters(std::string seperator);
   // std::vector<LanguageRuntime> all_runtimes();

private:
  LanguageRegistry() = default;
  static LanguageRegistry *inst;
  struct runtime_entry {
    LanguageRuntime *runtime{};
    bool active;
  };
  std::unordered_map<std::string, runtime_entry> entries;
};