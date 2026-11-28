#include "AIService.h"

#ifndef __EMSCRIPTEN__

#include "HTTPClient.h"
#include "AIClient.h"
#include "AIProvider.h"
#include "AIProfileManager.h"
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

std::vector<AIProviderPreset> AIService::getProviderPresets()
{
  return {
    {
      "OpenAI ChatGPT",
      "https://api.openai.com/v1",
      "gpt-6.2-sol",
      "OPENAI_API_KEY",
      "OpenAI Responses API"
    },
    {
      "Google Gemini",
      "https://generativelanguage.googleapis.com/v1beta",
      "gemini-3.8-flash",
      "GEMINI_API_KEY",
      "Gemini API; free tier availability depends on model, project and limits"
    },
    {
      "Anthropic Claude",
      "https://api.anthropic.com/v1",
      "claude-sonnet-5-5",
      "ANTHROPIC_API_KEY",
      "Anthropic Messages API; API credits or billing required"
    },
  };
}

void AIService::ensureDefaultProfiles()
{
  std::string path = getAISettingsPath();
  nlohmann::json root;
  bool fileExists = false;

  {
    std::ifstream in(path);
    if (in.is_open()) {
      try {
        in >> root;
        fileExists = true;
      } catch (...) {
        root = nlohmann::json::object();
      }
    }
  }

  bool modified = false;
  if (!root.contains("profiles") || !root["profiles"].is_object()) {
    root["profiles"] = nlohmann::json::object();
    modified = true;
  }

  auto& profiles = root["profiles"];
  auto presets = getProviderPresets();

  for (const auto& preset : presets) {
    if (!profiles.contains(preset.name)) {
      nlohmann::json p = nlohmann::json::object();
      p["endpoint"] = preset.endpoint;
      p["apiKey"] = "";
      nlohmann::json params = nlohmann::json::object();
      params["model"] = preset.defaultModel;
      params["temperature"] = 0.7;
      params["max_tokens"] = 4096;
      p["params"] = params;
      profiles[preset.name] = p;
      modified = true;
    } else if (profiles[preset.name].is_object()) {
      auto& profile = profiles[preset.name];
      const std::string oldEndpoint = profile.value("endpoint", std::string{});
      if (oldEndpoint.empty() || oldEndpoint == "https://generativelanguage.googleapis.com/v1beta/openai") {
        profile["endpoint"] = preset.endpoint;
        modified = true;
      }
      if (!profile.contains("params") || !profile["params"].is_object()) {
        profile["params"] = nlohmann::json::object();
        modified = true;
      }
      auto& params = profile["params"];
      const std::string oldModel = params.value("model", std::string{});
      if (oldModel.empty() || oldModel == "gpt-4o" || oldModel == "claude-3-7-sonnet-20250219") {
        params["model"] = preset.defaultModel;
        modified = true;
      }
      if (profile.contains("apiKey") && profile["apiKey"].is_string() && !profile["apiKey"].get<std::string>().empty()) {
        const std::string protectedKey = AIProfileManager::protectApiKey(profile["apiKey"].get<std::string>());
        if (!protectedKey.empty()) {
          profile["apiKeyEncrypted"] = protectedKey;
          profile.erase("apiKey");
          modified = true;
        }
      }
    }
  }

  if (!root.contains("activeProfile") || !root["activeProfile"].is_string() ||
      !profiles.contains(root["activeProfile"].get<std::string>())) {
    root["activeProfile"] = "Google Gemini";
    modified = true;
  }
  const std::string activeProfile = root["activeProfile"].get<std::string>();
  // Prefer the user's explicitly configured Gemini key when older settings
  // still point at the OpenRouter profile from the initial AI integration.
  if (activeProfile == "OpenRouter" || activeProfile == "Custom" || activeProfile == "custom") {
    root["activeProfile"] = "Google Gemini";
    modified = true;
  }
  if (modified || !fileExists) {
    std::ofstream out(path);
    if (out.is_open()) {
      out << root.dump(2);
    }
  }
}

std::vector<std::string> AIService::getAvailableProfileNames()
{
  ensureDefaultProfiles();
  std::vector<std::string> names;
  std::ifstream in(getAISettingsPath());
  if (!in.is_open()) return names;
  try {
    nlohmann::json root;
    in >> root;
    if (root.contains("profiles") && root["profiles"].is_object()) {
      for (const auto& preset : getProviderPresets()) {
        if (root["profiles"].contains(preset.name)) names.push_back(preset.name);
      }
    }
  } catch (...) {}
  return names;
}

std::vector<std::string> AIService::getConfiguredProfileNames()
{
  ensureDefaultProfiles();
  std::vector<std::string> names;
  std::ifstream in(getAISettingsPath());
  if (!in.is_open()) return names;
  try {
    nlohmann::json root;
    in >> root;
    if (root.contains("profiles") && root["profiles"].is_object()) {
      for (const auto& preset : getProviderPresets()) {
        if (!root["profiles"].contains(preset.name) || !root["profiles"][preset.name].is_object()) continue;
        const std::string apiKey = AIProfileManager::apiKeyFromProfile(root["profiles"][preset.name]);
        if (!apiKey.empty()) names.push_back(preset.name);
      }
    }
  } catch (...) {
  }
  return names;
}

std::string AIService::getProfileModelName(const std::string& profileName)
{
  ensureDefaultProfiles();
  std::ifstream in(getAISettingsPath());
  if (!in.is_open()) return {};
  try {
    nlohmann::json root;
    in >> root;
    const auto& profiles = root.at("profiles");
    const auto& profile = profiles.at(profileName);
    const auto& params = profile.at("params");
    if (params.contains("model") && params["model"].is_string()) {
      return params["model"].get<std::string>();
    }
  } catch (...) {
  }
  return {};
}

std::string AIService::getActiveProfileName()
{
  ensureDefaultProfiles();
  std::ifstream in(getAISettingsPath());
  if (!in.is_open()) return "Google Gemini";
  try {
    nlohmann::json root;
    in >> root;
    if (root.contains("activeProfile") && root["activeProfile"].is_string()) {
      return root["activeProfile"].get<std::string>();
    }
  } catch (...) {}
  return "Google Gemini";
}

bool AIService::setActiveProfileName(const std::string& name)
{
  ensureDefaultProfiles();
  std::string path = getAISettingsPath();
  nlohmann::json root;
  {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    try {
      in >> root;
    } catch (...) {
      return false;
    }
  }

  if (!root.contains("profiles") || !root["profiles"].contains(name)) {
    return false;
  }

  root["activeProfile"] = name;
  std::ofstream out(path);
  if (!out.is_open()) return false;
  out << root.dump(2);
  return true;
}

bool AIService::createProfileFromPreset(const std::string& presetName, const std::string& apiKey)
{
  auto presets = getProviderPresets();
  for (const auto& preset : presets) {
    if (preset.name == presetName) {
      std::string path = getAISettingsPath();
      nlohmann::json root;
      {
        std::ifstream in(path);
        if (in.is_open()) {
          try { in >> root; } catch (...) {}
        }
      }
      if (!root.contains("profiles") || !root["profiles"].is_object()) {
        root["profiles"] = nlohmann::json::object();
      }
      nlohmann::json p = nlohmann::json::object();
      p["endpoint"] = preset.endpoint;
      p["apiKeyEncrypted"] = AIProfileManager::protectApiKey(apiKey);
      nlohmann::json params = nlohmann::json::object();
      params["model"] = preset.defaultModel;
      params["temperature"] = 0.7;
      params["max_tokens"] = 4096;
      p["params"] = params;
      root["profiles"][preset.name] = p;
      root["activeProfile"] = preset.name;
      std::ofstream out(path);
      if (out.is_open()) {
        out << root.dump(2);
        return true;
      }
    }
  }
  return false;
}

static bool loadActiveProfile(AIProfileConfig& config, std::string& error_msg)
{
  AIService::ensureDefaultProfiles();
  std::ifstream file(getAISettingsPath());
  if (!file.is_open()) {
    error_msg = "Could not open ai_settings.json config file.";
    return false;
  }

  nlohmann::json j;
  try {
    file >> j;
  } catch (const std::exception& e) {
    error_msg = std::string("JSON parsing error: ") + e.what();
    return false;
  }

  if (!j.contains("activeProfile") || !j["activeProfile"].is_string()) {
    error_msg = "ai_settings.json is missing 'activeProfile' string.";
    return false;
  }
  std::string active_profile_name = j["activeProfile"].get<std::string>();

  if (!j.contains("profiles") || !j["profiles"].is_object()) {
    error_msg = "ai_settings.json is missing 'profiles' object.";
    return false;
  }
  auto& profiles = j["profiles"];

  if (!profiles.contains(active_profile_name) || !profiles[active_profile_name].is_object()) {
    error_msg = "Active profile '" + active_profile_name + "' was not found in 'profiles'.";
    return false;
  }
  auto& profile = profiles[active_profile_name];

  config.provider = active_profile_name;
  config.endpoint = profile.value("endpoint", AIProviderManager::defaultEndpoint(active_profile_name));
  if (config.endpoint.empty()) {
    error_msg = "API endpoint is empty. Please configure a valid API URL in Preferences -> AI tab.";
    return false;
  }
  if (config.endpoint.rfind("http://", 0) != 0 && config.endpoint.rfind("https://", 0) != 0) {
    error_msg =
      "Invalid API endpoint: '" + config.endpoint + "'. Must start with 'http://' or 'https://'";
    return false;
  }
  config.apiKey = AIProfileManager::apiKeyFromProfile(profile);

  // Fallback to environment variables if API key is not configured in file
  if (config.apiKey.empty()) {
    std::string lowerName = active_profile_name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    std::string lowerEndpoint = config.endpoint;
    std::transform(lowerEndpoint.begin(), lowerEndpoint.end(), lowerEndpoint.begin(), ::tolower);

    const char *env_val = nullptr;
    if (lowerName.find("openrouter") != std::string::npos || lowerEndpoint.find("openrouter") != std::string::npos) {
      env_val = std::getenv("OPENROUTER_API_KEY");
    } else if (lowerName.find("openai") != std::string::npos || lowerEndpoint.find("openai") != std::string::npos) {
      env_val = std::getenv("OPENAI_API_KEY");
    } else if (lowerName.find("gemini") != std::string::npos || lowerEndpoint.find("googleapis") != std::string::npos) {
      env_val = std::getenv("GEMINI_API_KEY");
      if (!env_val) env_val = std::getenv("GOOGLE_API_KEY");
    } else if (lowerName.find("claude") != std::string::npos || lowerName.find("anthropic") != std::string::npos || lowerEndpoint.find("anthropic") != std::string::npos) {
      env_val = std::getenv("ANTHROPIC_API_KEY");
      if (!env_val) env_val = std::getenv("CLAUDE_API_KEY");
    }
    if (!env_val) {
      env_val = std::getenv("AI_API_KEY");
    }
    if (env_val && std::strlen(env_val) > 0) {
      config.apiKey = env_val;
    }
  }

  if (profile.contains("params") && profile["params"].is_object()) {
    auto& params = profile["params"];
    config.model = params.value("model", "");
    config.parameters = params;
  } else {
    config.model = "";
    config.parameters = nlohmann::json::object();
  }

  if (config.model.empty()) config.model = AIProviderManager::defaultModel(active_profile_name);

  return true;
}

class AIService::Impl
{
public:
  std::unique_ptr<HTTPClient> http_client;
  std::unique_ptr<AIClient> ai_client;
  ToolExecutor tool_executor = nullptr;
  HistoryDrainCallback history_drain_callback = nullptr;

  Impl()
  {
    http_client = std::make_unique<HTTPClient>();
    ai_client = std::make_unique<AIClient>(std::shared_ptr<HTTPClient>(http_client.release()));
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

void AIService::registerToolExecutor(ToolExecutor executor)
{
  impl->tool_executor = std::move(executor);
}

void AIService::registerHistoryDrainCallback(HistoryDrainCallback callback)
{
  impl->history_drain_callback = std::move(callback);
}

void AIService::chatCompletionStream(std::vector<ChatMessage>& history, ChunkCallback on_chunk,
                                     ErrorCallback on_error, CompleteCallback on_complete,
                                     bool interactiveMode, int current_auto_turn)
{
  AIProfileConfig config;
  std::string error_msg;
  if (!loadActiveProfile(config, error_msg)) {
    if (on_error) {
      on_error(error_msg);
    }
    return;
  }

  int max_auto_turns = 5;
  if (config.parameters.contains("max_auto_turns") &&
      config.parameters["max_auto_turns"].is_number_integer()) {
    max_auto_turns = config.parameters["max_auto_turns"].get<int>();
    if (max_auto_turns < 1) max_auto_turns = 1;
  }

  std::vector<AIChatMessage> ai_history;
  std::string sys_prompt =
    "You are the OpenSCAD Expert Assistant. You can create complete 3D models of ANY object from scratch, write high-quality OpenSCAD code, and provide surgical fixes.\n\n"
    "### YOUR CORE RULES:\n"
    "1. **3D Model Creation & Generation**: When the user asks to create, model, generate, or design ANY 3D object whatsoever (e.g. vehicles like cars, buses, or planes; furniture like chairs, tables, or desks; household items like mugs, vases, or lamps; tools & gadgets like phone stands, brackets, or clips; mechanical parts like gears, pulleys, or bolts; buildings like houses or castles; or any other custom geometry):\n"
    "   - Create a complete, detailed, fully recognizable 3D model using OpenSCAD primitives (`cube`, `cylinder`, `sphere`), transformations (`translate`, `rotate`), CSG boolean operations (`difference`, `union`, `intersection`), and rich colors (`color(...)`).\n"
    "   - Structure the model into clear, modular OpenSCAD modules with parameters and comments (e.g. `module main_body()`, `module details()`).\n"
    "   - Set sensible `$fn` (e.g. `$fn = 50;`) for smooth curved surfaces.\n"
    "   - You MUST call `set_editor_code` with the complete generated code to stage it for manual user review.\n"
    "   - Never edit the active editor directly; the user edits and accepts proposed code manually.\n"
    "2. **Surgical Excellence**: If the user has an existing script with a minor error (missing semicolon, wrong bracket), fix "
    "ONLY that specific line. Do NOT rewrite the entire script, do NOT rename variables, and do NOT "
    "change the overall logic unless explicitly asked.\n"
    "3. **OpenSCAD Syntax Mastery**:\n"
    "   - **Modifiers**: `color()`, `rotate()`, `translate()`, etc., are MODIFIERS. They apply to the "
    "next child or block. NEVER assign them to variables like `c = color(\"red\");`. Instead, use "
    "`color(\"red\") cube(10);`.\n"
    "   - **Semicolons**: Every assignment (e.g., `x = 5;`) and every module instantiation (e.g., "
    "`cube(10);`) MUST end with a semicolon. Semicolons are NOT used after module definitions `module "
    "name() { ... }` or after blocks `{ ... }`.\n"
    "3. **Tool Workflow**:\n"
    "   - YOU MUST USE `set_editor_code` to propose code changes for manual review.\n"
    "   - You can output code blocks in markdown format in the chat for explanation, but you MUST also "
    "call `set_editor_code` to update the script.\n"
    "   - Use `get_editor_code()` if you need to see the latest script state.\n"
    "   - Use `trigger_preview()` after setting code to validate compilation and check for errors.\n"
    "   - Use `get_viewport()` and `set_camera(...)` to inspect or adjust 3D view camera angles.\n"
    "4. **Response and Engagement**: Explain the reasoning behind your proposed code changes. Output "
    "standard text to explain your thoughts and keep the user engaged while proposing code changes via "
    "tools.\n"
    "5. **Formatting**: Use ACTUAL NEWLINES in your code output. Never use literal '\\n' sequences.\n"
    "6. **Tone**: Technical, concise, and helpful. Avoid long conversational filler.";

  if (config.parameters.contains("system_prompt") && config.parameters["system_prompt"].is_string()) {
    sys_prompt = config.parameters["system_prompt"].get<std::string>();
  }

  // Append mode-specific instructions so the model understands its operating context
  if (interactiveMode) {
    sys_prompt +=
      "\n\n### Current Operating Mode: INTERACTIVE REVIEW\n"
      "You are in interactive review mode. Follow these rules strictly:\n"
      "- Read the script with `get_editor_code()` if needed, then call `set_editor_code` ONCE with "
      "your proposed changes.\n"
      "- After calling `set_editor_code`, STOP. Do NOT call `trigger_preview` yourself. "
      "The user will review your diff and the preview will fire automatically upon acceptance.\n"
      "- Do NOT loop or call further tools after proposing code.";
  } else {
    sys_prompt +=
      "\n\n### Current Operating Mode: AUTONOMOUS AGENTIC\n"
      "You are in autonomous mode. Follow these rules:\n"
      "- Call `set_editor_code` to stage a proposal, then STOP for manual user review. Never write directly to the editor.\n"
      "- Do not call `trigger_preview`; the user edits and accepts the proposal manually.";
  }

  int context_limit = 10;
  if (config.parameters.contains("context_limit") &&
      config.parameters["context_limit"].is_number_integer()) {
    context_limit = config.parameters["context_limit"].get<int>();
    if (context_limit < 1) context_limit = 1;
  }

  bool already_has_system = false;
  if (!history.empty() && history[0].role == "system") {
    already_has_system = true;
  }
  if (!already_has_system) {
    // Append hidden OpenSCAD reference documentation to the system prompt
    std::string ref_doc =
      "\n\n### OpenSCAD Quick Reference\n"
      "**3D Primitives**: cube([x,y,z], center), sphere(r, $fn), cylinder(h, r1, r2, center, $fn)\n"
      "**2D Primitives**: circle(r, $fn), square([x,y], center), polygon(points, paths), text(t, size, "
      "font, halign, valign)\n"
      "**Transforms**: translate([x,y,z]), rotate([x,y,z]), scale([x,y,z]), mirror([x,y,z]), "
      "multmatrix(m), color(c, alpha), offset(r|delta, chamfer), hull(), minkowski()\n"
      "**Boolean Ops**: union(), difference(), intersection()\n"
      "**Extrusion**: linear_extrude(height, center, twist, slices, scale, $fn), "
      "rotate_extrude(angle, $fn)\n"
      "**Modules**: module name(params) { body } -- NO semicolon after definition. "
      "Call with: name(args);\n"
      "**Functions**: function name(params) = expression;\n"
      "**Control**: for(i=[start:step:end]), if(cond), let(assignments), each, "
      "assert(cond, msg), echo(values)\n"
      "**Math**: sin, cos, tan, asin, acos, atan, atan2, abs, ceil, floor, round, "
      "sqrt, pow, exp, ln, log, min, max, norm, cross\n"
      "**List Ops**: concat, len, lookup, str, chr, ord, search, flatten\n"
      "**Special Vars**: $fn (fragments), $fa (fragment angle), $fs (fragment size), "
      "$t (animation), $vpr/$vpt/$vpd/$vpf (viewport)\n"
      "**Import/Use**: use <file.scad> (functions/modules only), include <file.scad> (full), "
      "import(\"file.stl\")\n\n"
      "### Available Tools\n"
      "- **get_editor_code()**: Returns the current contents of the active editor tab.\n"
      "- **set_editor_code({\"code\": \"...\"})**: Applies code changes directly to active editor.\n"
      "- **trigger_preview()**: Renders current editor code and returns compile errors/warnings.\n"
      "- **get_viewport()**: Returns 3D viewport camera parameters (vpt, vpr, vpd, vpf, projection).\n"
      "- **set_camera({\"vpt\": [x,y,z], \"vpr\": [x,y,z], ...})**: Sets 3D view camera vectors.";
    ai_history.push_back({"system", sys_prompt + ref_doc});
  }

  // Apply sliding window: keep last context_limit turn pairs (a turn starts with a "user" message)
  size_t window_start = 0;
  {
    std::vector<size_t> turn_starts;
    for (size_t i = 0; i < history.size(); ++i) {
      if (history[i].role == "user") {
        turn_starts.push_back(i);
      }
    }
    if (static_cast<int>(turn_starts.size()) > context_limit) {
      window_start = turn_starts[turn_starts.size() - context_limit];
    }
  }

  for (size_t i = window_start; i < history.size(); ++i) {
    const auto& msg = history[i];
    AIChatMessage am;
    am.role = msg.role;
    am.content = msg.content;
    am.tool_call_id = msg.tool_call_id;
    am.tool_name = msg.tool_name;
    for (const auto& img : msg.images) {
      am.images.push_back({img.mime_type, img.base64_data});
    }
    if (!msg.tool_calls.empty()) {
      try {
        auto tcs_json = nlohmann::json::parse(msg.tool_calls);
        if (tcs_json.is_array()) {
          for (auto& tc_json : tcs_json) {
            AIToolCall tc;
            if (tc_json.contains("id")) tc.id = tc_json["id"].get<std::string>();
            if (tc_json.contains("type")) tc.type = tc_json["type"].get<std::string>();
            if (tc_json.contains("function") && tc_json["function"].is_object()) {
              auto& fn = tc_json["function"];
              if (fn.contains("name")) tc.name = fn["name"].get<std::string>();
              if (fn.contains("arguments")) {
                if (fn["arguments"].is_string()) {
                  tc.arguments = fn["arguments"].get<std::string>();
                } else {
                  tc.arguments = fn["arguments"].dump();
                }
              }
            }
            am.tool_calls.push_back(tc);
          }
        }
      } catch (...) {
      }
    }
    ai_history.push_back(am);
  }

  // In interactive mode the model gets at most 2 LLM calls: one to call tools
  // (e.g. get_editor_code then set_editor_code) and one final response after
  // seeing those results. We enforce this by capping the effective turn limit.
  // A model creation normally needs one call to write code and one to render;
  // allow one final response while avoiding long agent loops for simple prompts.
  const int effective_max_turns = std::min(max_auto_turns, interactiveMode ? 2 : 3);

  auto on_complete_wrapper = [this, config, &history, on_chunk, on_error, on_complete, current_auto_turn,
                              effective_max_turns,
                              interactiveMode](const std::vector<AIToolCall>& tool_calls) {
    if (tool_calls.empty()) {
      if (on_complete) {
        on_complete();
      }
      return;
    }

    ChatMessage assistant_msg;
    assistant_msg.role = "assistant";
    assistant_msg.content = "";
    nlohmann::json tcs_arr = nlohmann::json::array();
    for (const auto& tc : tool_calls) {
      nlohmann::json t = nlohmann::json::object();
      const std::string toolCallId =
        (config.endpoint.find("generativelanguage.googleapis.com") != std::string::npos && tc.id.empty())
          ? "call_" + tc.name
          : tc.id;
      t["id"] = toolCallId;
      t["type"] = "function";
      nlohmann::json fn = nlohmann::json::object();
      fn["name"] = tc.name;
      fn["arguments"] = tc.arguments;
      t["function"] = fn;
      tcs_arr.push_back(t);
    }
    assistant_msg.tool_calls = tcs_arr.dump();
    history.push_back(assistant_msg);

    for (const auto& tc : tool_calls) {
      std::string result;
      if (impl->tool_executor) {
        result = impl->tool_executor(tc.name, tc.arguments);
      } else {
        result = "Error: No tool executor registered.";
      }
      ChatMessage tool_msg;
      tool_msg.role = "tool";
      tool_msg.content = result;
      tool_msg.tool_call_id =
        (config.endpoint.find("generativelanguage.googleapis.com") != std::string::npos && tc.id.empty())
          ? "call_" + tc.name
          : tc.id;
      tool_msg.tool_name = tc.name;
      history.push_back(tool_msg);
    }

    // Staged editor changes always return control to the user for manual review.
    const bool stagedCode = std::any_of(tool_calls.begin(), tool_calls.end(),
                                        [](const AIToolCall& call) {
                                          return call.name == "set_editor_code";
                                        });
    if (stagedCode && interactiveMode) {
      if (on_complete) on_complete();
      return;
    }

    // Allow the UI layer to inject additional context messages (e.g. a
    // viewport snapshot captured after trigger_preview) into history before
    // the next recursive LLM call.
    if (impl->history_drain_callback) {
      impl->history_drain_callback(history);
    }

    if (current_auto_turn + 1 >= effective_max_turns) {
      if (on_complete) {
        on_complete();
      }
      return;
    }

    chatCompletionStream(history, on_chunk, on_error, on_complete, interactiveMode,
                         current_auto_turn + 1);
  };

  impl->ai_client->sendChatCompletionStream(config, ai_history, on_chunk, on_error, on_complete_wrapper);
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
    "You are the OpenSCAD Expert Assistant. You can create complete 3D models of ANY object from scratch, write high-quality OpenSCAD code, and provide surgical fixes.\n\n"
    "### YOUR CORE RULES:\n"
    "1. **3D Model Creation & Generation**: When the user asks to create, model, generate, or design ANY 3D object whatsoever (e.g. vehicles like cars, buses, or planes; furniture like chairs, tables, or desks; household items like mugs, vases, or lamps; tools & gadgets like phone stands, brackets, or clips; mechanical parts like gears, pulleys, or bolts; buildings like houses or castles; or any other custom geometry):\n"
    "   - Create a complete, detailed, fully recognizable 3D model using OpenSCAD primitives (`cube`, `cylinder`, `sphere`), transformations (`translate`, `rotate`), CSG boolean operations (`difference`, `union`, `intersection`), and rich colors (`color(...)`).\n"
    "   - Structure the model into clear, modular OpenSCAD modules with parameters and comments (e.g. `module main_body()`, `module details()`).\n"
    "   - Set sensible `$fn` (e.g. `$fn = 50;`) for smooth curved surfaces.\n"
    "   - You MUST call `set_editor_code` with the complete generated code to place it directly into the editor.\n"
    "   - In autonomous mode, call `trigger_preview()` immediately after `set_editor_code` to compile and display the 3D model in the viewport.\n"
    "2. **Surgical Excellence**: If the user has an existing script with a minor error (missing semicolon, wrong bracket), fix "
    "ONLY that specific line. Do NOT rewrite the entire script, do NOT rename variables, and do NOT "
    "change the overall logic unless explicitly asked.\n"
    "3. **OpenSCAD Syntax Mastery**:\n"
    "   - **Modifiers**: `color()`, `rotate()`, `translate()`, etc., are MODIFIERS. They apply to the "
    "next child or block. NEVER assign them to variables like `c = color(\"red\");`. Instead, use "
    "`color(\"red\") cube(10);`.\n"
    "   - **Semicolons**: Every assignment (e.g., `x = 5;`) and every module instantiation (e.g., "
    "`cube(10);`) MUST end with a semicolon. Semicolons are NOT used after module definitions `module "
    "name() { ... }` or after blocks `{ ... }`.\n"
    "3. **Tool Workflow**:\n"
    "   - YOU MUST USE `set_editor_code` to apply any code changes in the active editor.\n"
    "   - You can output code blocks in markdown format in the chat for explanation, but you MUST also "
    "call the `set_editor_code` tool so the changes are set for review and can be applied "
    "automatically.\n"
    "   - Use `get_editor_code()` if you need to see the latest script state.\n"
    "   - Use `trigger_preview()` once after setting the code to validate the result.\n"
    "4. **Response and Engagement**: Explain the reasoning behind your proposed code changes. Output "
    "standard text to explain your thoughts and keep the user engaged while proposing code changes via "
    "tools.\n"
    "5. **Formatting**: Use ACTUAL NEWLINES in your code output. Never use literal '\\n' sequences.\n"
    "6. **Tone**: Technical, concise, and helpful. Avoid long conversational filler.";
  if (config.parameters.contains("system_prompt") && config.parameters["system_prompt"].is_string()) {
    sys_prompt = config.parameters["system_prompt"].get<std::string>();
  }

  bool already_has_system = false;
  if (!history.empty() && history[0].role == "system") {
    already_has_system = true;
  }
  if (!already_has_system) {
    // Append hidden OpenSCAD reference documentation to the system prompt
    std::string ref_doc =
      "\n\n### OpenSCAD Quick Reference\n"
      "**3D Primitives**: cube([x,y,z], center), sphere(r, $fn), cylinder(h, r1, r2, center, $fn)\n"
      "**2D Primitives**: circle(r, $fn), square([x,y], center), polygon(points, paths), text(t, size, "
      "font, halign, valign)\n"
      "**Transforms**: translate([x,y,z]), rotate([x,y,z]), scale([x,y,z]), mirror([x,y,z]), "
      "multmatrix(m), color(c, alpha), offset(r|delta, chamfer), hull(), minkowski()\n"
      "**Boolean Ops**: union(), difference(), intersection()\n"
      "**Extrusion**: linear_extrude(height, center, twist, slices, scale, $fn), "
      "rotate_extrude(angle, $fn)\n"
      "**Modules**: module name(params) { body } -- NO semicolon after definition. "
      "Call with: name(args);\n"
      "**Functions**: function name(params) = expression;\n"
      "**Control**: for(i=[start:step:end]), if(cond), let(assignments), each, "
      "assert(cond, msg), echo(values)\n"
      "**Math**: sin, cos, tan, asin, acos, atan, atan2, abs, ceil, floor, round, "
      "sqrt, pow, exp, ln, log, min, max, norm, cross\n"
      "**List Ops**: concat, len, lookup, str, chr, ord, search, flatten\n"
      "**Special Vars**: $fn (fragments), $fa (fragment angle), $fs (fragment size), "
      "$t (animation), $vpr/$vpt/$vpd/$vpf (viewport)\n"
      "**Import/Use**: use <file.scad> (functions/modules only), include <file.scad> (full), "
      "import(\"file.stl\")\n\n"
      "### Available Tools\n"
      "- **get_editor_code()**: Returns the current contents of the active editor tab.\n"
      "- **set_editor_code({\"code\": \"...\"})**: Applies code in the editor in agent mode, or proposes it for review in interactive mode. "
      "Always use this instead of pasting code in chat.\n"
      "- **trigger_preview()**: Renders the current editor code in the 3D viewport. "
      "Automatically postponed if code changes are pending review.";
    ai_history.push_back({"system", sys_prompt + ref_doc});
  }

  for (const auto& msg : history) {
    AIChatMessage am;
    am.role = msg.role;
    am.content = msg.content;
    am.tool_call_id = msg.tool_call_id;
    am.tool_name = msg.tool_name;
    for (const auto& img : msg.images) {
      am.images.push_back({img.mime_type, img.base64_data});
    }
    if (!msg.tool_calls.empty()) {
      try {
        auto tcs_json = nlohmann::json::parse(msg.tool_calls);
        if (tcs_json.is_array()) {
          for (auto& tc_json : tcs_json) {
            AIToolCall tc;
            if (tc_json.contains("id")) tc.id = tc_json["id"].get<std::string>();
            if (tc_json.contains("type")) tc.type = tc_json["type"].get<std::string>();
            if (tc_json.contains("function") && tc_json["function"].is_object()) {
              auto& fn = tc_json["function"];
              if (fn.contains("name")) tc.name = fn["name"].get<std::string>();
              if (fn.contains("arguments")) {
                if (fn["arguments"].is_string()) {
                  tc.arguments = fn["arguments"].get<std::string>();
                } else {
                  tc.arguments = fn["arguments"].dump();
                }
              }
            }
            am.tool_calls.push_back(tc);
          }
        }
      } catch (...) {
      }
    }
    ai_history.push_back(am);
  }

  impl->ai_client->sendChatCompletion(
    config, ai_history,
    [on_response](const std::string& response, const std::vector<AIToolCall>&) {
      if (on_response) {
        on_response(response);
      }
    },
    on_error);
}

void AIService::generateOpenSCADCode(const std::string& prompt, const std::string& currentCode,
                                     ChunkCallback on_chunk, ErrorCallback on_error,
                                     CompleteCallback on_complete)
{
  AIProfileConfig config;
  std::string error;
  if (!loadActiveProfile(config, error)) {
    if (on_error) on_error(error);
    return;
  }
  if (config.apiKey.empty()) {
    if (on_error) on_error("No API key is configured for " + getActiveProfileName() +
                           ". Add its provider API key in Preferences → AI.");
    return;
  }
  if (config.model.empty()) {
    if (on_error) on_error("No model is configured for this provider in Preferences → AI.");
    return;
  }

  const std::string systemPrompt =
    "You generate valid, complete OpenSCAD source code from the user's request. "
    "Return only OpenSCAD code, without Markdown fences or explanation. "
    "OpenSCAD has no physical unit system: use millimeters for dimensions and convert meters to millimeters "
    "(1 meter = 1000 mm). Preserve existing code and modify it only when the request asks to edit the current model. "
    "Use valid OpenSCAD syntax, sensible parameters, and avoid external dependencies or imports. "
    "For a new model, return a complete standalone script that can be pasted into the editor and rendered. "
    "For edits, return the complete updated script. Do not attempt to call tools, edit files, or render.";
  std::string userPrompt = prompt;
  if (!currentCode.empty()) {
    userPrompt += "\n\nCurrent OpenSCAD editor source (use as context for requested edits):\n```scad\n";
    userPrompt += currentCode;
    userPrompt += "\n```";
  }
  impl->ai_client->generateOpenSCADCodeStream(config, systemPrompt, userPrompt,
                                              std::move(on_chunk), std::move(on_error),
                                              std::move(on_complete));
}

void AIService::testConnection(CompleteCallback on_success, ErrorCallback on_error)
{
  AIProfileConfig config;
  std::string error;
  if (!loadActiveProfile(config, error)) {
    if (on_error) on_error(error);
    return;
  }
  if (config.apiKey.empty()) {
    if (on_error) on_error("Paste an API key for the selected provider before testing the connection.");
    return;
  }
  if (config.model.empty()) {
    if (on_error) on_error("Select a model before testing the connection.");
    return;
  }
  impl->ai_client->testConnection(config, std::move(on_success), std::move(on_error));
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

void AIService::cancelPendingRequests()
{
  impl->ai_client->cancelPendingRequests();
}

int AIService::getPayloadLimit() const
{
  AIProfileConfig config;
  std::string error_msg;
  if (loadActiveProfile(config, error_msg)) {
    if (config.parameters.contains("payload_limit") &&
        config.parameters["payload_limit"].is_number_integer()) {
      return config.parameters["payload_limit"].get<int>();
    }
  }
  return 50000;
}

bool AIService::getAutoAttachViewport() const
{
  AIProfileConfig config;
  std::string error_msg;
  if (loadActiveProfile(config, error_msg)) {
    if (config.parameters.contains("auto_attach_viewport") &&
        config.parameters["auto_attach_viewport"].is_boolean()) {
      return config.parameters["auto_attach_viewport"].get<bool>();
    }
  }
  return false;
}

int AIService::getMaxAutoTurns() const
{
  AIProfileConfig config;
  std::string error_msg;
  if (loadActiveProfile(config, error_msg)) {
    if (config.parameters.contains("max_auto_turns") &&
        config.parameters["max_auto_turns"].is_number_integer()) {
      int val = config.parameters["max_auto_turns"].get<int>();
      if (val >= 1) return val;
    }
  }
  return 5;
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

void AIService::chatCompletionStream(std::vector<ChatMessage>&, ChunkCallback, ErrorCallback on_error,
                                     CompleteCallback, bool, int)
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

void AIService::cancelPendingRequests()
{
}

int AIService::getPayloadLimit() const
{
  return 50000;
}

bool AIService::getAutoAttachViewport() const
{
  return false;
}

int AIService::getMaxAutoTurns() const
{
  return 5;
}

#endif  // __EMSCRIPTEN__
