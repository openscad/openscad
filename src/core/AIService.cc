#include "AIService.h"

#ifndef __EMSCRIPTEN__

#include "HTTPClient.h"
#include "AIClient.h"
#include "platform/PlatformUtils.h"
#include "json/json.hpp"
#include <fstream>
#include <cstdlib>

static std::string getAISettingsPath()
{
  std::string configPath = PlatformUtils::userConfigPath();
  if (configPath.empty()) {
    char *home = std::getenv("HOME");
    if (home) {
      configPath = std::string(home) + "/.openscad";
    } else {
      configPath = ".";
    }
  }
  return configPath + "/ai_settings.json";
}

static nlohmann::json readAISettings()
{
  std::string path = getAISettingsPath();
  std::ifstream file(path);
  if (!file.is_open()) {
    return nlohmann::json::object();
  }
  nlohmann::json j;
  try {
    file >> j;
  } catch (...) {
    return nlohmann::json::object();
  }
  return j;
}

static bool loadActiveProfile(AIProfileConfig& config, std::string& error_msg)
{
  nlohmann::json settings = readAISettings();
  if (settings.empty() || !settings.is_object()) {
    error_msg = "No AI settings found. Please configure AI in Preferences.";
    return false;
  }

  std::string activeProfile;
  if (settings.contains("activeProfile") && settings["activeProfile"].is_string()) {
    activeProfile = settings["activeProfile"].get<std::string>();
  }

  if (activeProfile.empty()) {
    error_msg = "No active AI profile selected.";
    return false;
  }

  if (!settings.contains("profiles") || !settings["profiles"].is_object()) {
    error_msg = "No profiles found in settings.";
    return false;
  }

  auto profiles = settings["profiles"];
  if (!profiles.contains(activeProfile) || !profiles[activeProfile].is_object()) {
    error_msg = "Active profile '" + activeProfile + "' not found in profiles.";
    return false;
  }

  auto prof = profiles[activeProfile];
  if (prof.contains("endpoint") && prof["endpoint"].is_string()) {
    config.endpoint = prof["endpoint"].get<std::string>();
  }
  if (prof.contains("apiKey") && prof["apiKey"].is_string()) {
    config.apiKey = prof["apiKey"].get<std::string>();
  }

  if (prof.contains("params") && prof["params"].is_object()) {
    auto params = prof["params"];
    if (params.contains("model") && params["model"].is_string()) {
      config.model = params["model"].get<std::string>();
    } else {
      config.model = "custom";
    }
    config.parameters = params;
  } else {
    config.model = "custom";
    config.parameters = nlohmann::json::object();
  }

  if (config.endpoint.empty()) {
    error_msg = "Endpoint is not configured for profile '" + activeProfile + "'.";
    return false;
  }

  return true;
}

class AIService::Impl
{
public:
  std::shared_ptr<HTTPClient> http_client;
  std::shared_ptr<AIClient> ai_client;

  Impl()
  {
    http_client = std::make_shared<HTTPClient>();
    ai_client = std::make_shared<AIClient>(http_client);
  }
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;
};

AIService::AIService() : impl(std::make_unique<Impl>())
{
}

AIService::~AIService() = default;

AIService::AIService(AIService&&) noexcept = default;
AIService& AIService::operator=(AIService&&) noexcept = default;

void AIService::chatCompletionStream(const std::vector<ChatMessage>& history, ChunkCallback on_chunk,
                                     ErrorCallback on_error, CompleteCallback on_complete)
{
  AIProfileConfig config;
  std::string error_msg;
  if (!loadActiveProfile(config, error_msg)) {
    if (on_error) {
      on_error(error_msg);
    }
    return;
  }

  std::vector<AIChatMessage> ai_history;
  std::string sys_prompt =
    "You are the OpenSCAD Expert Assistant. You provide high-quality, surgical, and logical OpenSCAD "
    "code fixes.\n\n"
    "### YOUR CORE RULES:\n"
    "1. **Surgical Excellence**: If the user has a minor error (missing semicolon, wrong bracket), fix "
    "ONLY that specific line. Do NOT rewrite the entire script, do NOT rename variables, and do NOT "
    "change the overall logic unless explicitly asked.\n"
    "2. **OpenSCAD Syntax Mastery**:\n"
    "   - **Modifiers**: `color()`, `rotate()`, `translate()`, etc., are MODIFIERS. They apply to the "
    "next child or block. NEVER assign them to variables like `c = color(\"red\");`. Instead, use "
    "`color(\"red\") cube(10);`.\n"
    "   - **Semicolons**: Every assignment (e.g., `x = 5;`) and every module instantiation (e.g., "
    "`cube(10);`) MUST end with a semicolon. Semicolons are NOT used after module definitions `module "
    "name() { ... }` or after blocks `{ ... }`.\n"
    "3. **Tool Workflow**:\n"
    "   - YOU MUST USE `set_editor_code` TO PROPOSE ANY CODE CHANGES.\n"
    "   - NEVER output OpenSCAD code in markdown block format. ALWAYS use the `set_editor_code` tool so "
    "the user can apply it.\n"
    "   - Use `get_editor_code()` if you need to see the latest script state.\n"
    "   - Use `trigger_preview()` once after setting the code to validate the result.\n"
    "4. **Response and Engagement**: Explain the reasoning behind your proposed code changes. Output "
    "standard text to explain your thoughts and keep the user engaged while proposing code changes via "
    "tools.\n"
    "5. **Formatting**: Use ACTUAL NEWLINES in your code output. Never use literal '\\n' sequences.\n"
    "6. **Tone**: Technical, concise, and helpful. Avoid long conversational filler.";
  if (config.parameters.contains("system_prompt") && config.parameters["system_prompt"].is_string()) {
    std::string temp = config.parameters["system_prompt"].get<std::string>();
    if (!temp.empty()) {
      sys_prompt = temp;
    }
  }

  bool already_has_system = false;
  if (!history.empty() && history[0].role == "system") {
    already_has_system = true;
  }
  if (!already_has_system) {
    ai_history.push_back({"system", sys_prompt});
  }

  for (const auto& msg : history) {
    ai_history.push_back({msg.role, msg.content});
  }

  impl->ai_client->sendChatCompletionStream(config, ai_history, on_chunk, on_error, on_complete);
}

void AIService::chatCompletion(const std::vector<ChatMessage>& history, ResponseCallback on_response,
                               ErrorCallback on_error)
{
  AIProfileConfig config;
  std::string error_msg;
  if (!loadActiveProfile(config, error_msg)) {
    if (on_error) {
      on_error(error_msg);
    }
    return;
  }

  std::vector<AIChatMessage> ai_history;
  std::string sys_prompt =
    "You are the OpenSCAD Expert Assistant. You provide high-quality, surgical, and logical OpenSCAD "
    "code fixes.\n\n"
    "### YOUR CORE RULES:\n"
    "1. **Surgical Excellence**: If the user has a minor error (missing semicolon, wrong bracket), fix "
    "ONLY that specific line. Do NOT rewrite the entire script, do NOT rename variables, and do NOT "
    "change the overall logic unless explicitly asked.\n"
    "2. **OpenSCAD Syntax Mastery**:\n"
    "   - **Modifiers**: `color()`, `rotate()`, `translate()`, etc., are MODIFIERS. They apply to the "
    "next child or block. NEVER assign them to variables like `c = color(\"red\");`. Instead, use "
    "`color(\"red\") cube(10);`.\n"
    "   - **Semicolons**: Every assignment (e.g., `x = 5;`) and every module instantiation (e.g., "
    "`cube(10);`) MUST end with a semicolon. Semicolons are NOT used after module definitions `module "
    "name() { ... }` or after blocks `{ ... }`.\n"
    "3. **Tool Workflow**:\n"
    "   - YOU MUST USE `set_editor_code` TO PROPOSE ANY CODE CHANGES.\n"
    "   - NEVER output OpenSCAD code in markdown block format. ALWAYS use the `set_editor_code` tool so "
    "the user can apply it.\n"
    "   - Use `get_editor_code()` if you need to see the latest script state.\n"
    "   - Use `trigger_preview()` once after setting the code to validate the result.\n"
    "4. **Response and Engagement**: Explain the reasoning behind your proposed code changes. Output "
    "standard text to explain your thoughts and keep the user engaged while proposing code changes via "
    "tools.\n"
    "5. **Formatting**: Use ACTUAL NEWLINES in your code output. Never use literal '\\n' sequences.\n"
    "6. **Tone**: Technical, concise, and helpful. Avoid long conversational filler.";
  if (config.parameters.contains("system_prompt") && config.parameters["system_prompt"].is_string()) {
    std::string temp = config.parameters["system_prompt"].get<std::string>();
    if (!temp.empty()) {
      sys_prompt = temp;
    }
  }

  bool already_has_system = false;
  if (!history.empty() && history[0].role == "system") {
    already_has_system = true;
  }
  if (!already_has_system) {
    ai_history.push_back({"system", sys_prompt});
  }

  for (const auto& msg : history) {
    ai_history.push_back({msg.role, msg.content});
  }

  impl->ai_client->sendChatCompletion(config, ai_history, on_response, on_error);
}

std::string AIService::getDefaultPrompt() const
{
  AIProfileConfig config;
  std::string error_msg;
  if (loadActiveProfile(config, error_msg)) {
    if (config.parameters.contains("default_prompt") &&
        config.parameters["default_prompt"].is_string()) {
      std::string prompt = config.parameters["default_prompt"].get<std::string>();
      if (!prompt.empty()) {
        return prompt;
      }
    }
  }
  return "Create a sphere with radius 10 and detail level $fn=50.";
}

#else  // __EMSCRIPTEN__

class AIService::Impl
{
};

AIService::AIService() : impl(std::make_unique<Impl>())
{
}
AIService::~AIService() = default;

AIService::AIService(AIService&&) noexcept = default;
AIService& AIService::operator=(AIService&&) noexcept = default;

void AIService::chatCompletionStream(const std::vector<ChatMessage>&, ChunkCallback,
                                     ErrorCallback on_error, CompleteCallback)
{
  if (on_error) {
    on_error("AI service is not supported on WebAssembly.");
  }
}

void AIService::chatCompletion(const std::vector<ChatMessage>&, ResponseCallback, ErrorCallback on_error)
{
  if (on_error) {
    on_error("AI service is not supported on WebAssembly.");
  }
}

std::string AIService::getDefaultPrompt() const
{
  return "";
}

#endif  // __EMSCRIPTEN__
