#pragma once
#include "Identifier.h"

std::unordered_map<std::string, std::string> Identifier::interned_names;

void Identifier::update(const std::string &name)
{
  hash = std::hash<std::string>()(name);
  is_config_variable_ = name[0] == '$' && name != "$children";

  auto it = interned_names.find(name);
  if (it != interned_names.end()) {
    interned_name_ptr = &it->second;
  } else {
    interned_name_ptr = &(interned_names[name] = name);
  }
}