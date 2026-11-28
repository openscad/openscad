#ifndef OPENSCAD_AIPROFILEMANAGER_H
#define OPENSCAD_AIPROFILEMANAGER_H

#include <string>
#include "json/json.hpp"

class AIProfileManager
{
public:
  static bool secureStorageAvailable();
  static std::string protectApiKey(const std::string& apiKey);
  static std::string revealApiKey(const std::string& storedValue);
  static std::string apiKeyFromProfile(const nlohmann::json& profile);
};

#endif
