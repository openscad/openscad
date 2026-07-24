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

static bool loadActiveProfile(AIProfileConfig& config, std::string& error_msg)
{
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

  config.endpoint = profile.value("endpoint", "");
  if (config.endpoint.empty()) {
    error_msg = "API endpoint is empty. Please configure a valid API URL in Preferences -> AI tab.";
    return false;
  }
  if (config.endpoint.rfind("http://", 0) != 0 && config.endpoint.rfind("https://", 0) != 0) {
    error_msg =
      "Invalid API endpoint: '" + config.endpoint + "'. Must start with 'http://' or 'https://'";
    return false;
  }
  config.apiKey = profile.value("apiKey", "");

  if (profile.contains("params") && profile["params"].is_object()) {
    auto& params = profile["params"];
    config.model = params.value("model", "");
    config.parameters = params;
  } else {
    config.model = "";
    config.parameters = nlohmann::json::object();
  }

  return true;
}

class AIService::Impl
{
public:
  std::unique_ptr<HTTPClient> http_client;
  std::unique_ptr<AIClient> ai_client;
  ToolExecutor tool_executor = nullptr;

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

void AIService::chatCompletionStream(std::vector<ChatMessage>& history, ChunkCallback on_chunk,
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
    "   - YOU MUST USE `set_editor_code` to propose any code changes so they are set for user review.\n"
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
    ai_history.push_back({"system", sys_prompt});
  }

  // Attach hidden OpenSCAD reference documentation
  std::string ref_doc =
    "### OpenSCAD Quick Reference\n"
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
    "- **set_editor_code({\"code\": \"...\"})**: Proposes code changes for user review in a "
    "side-by-side diff dialog. "
    "Always use this instead of pasting code in chat.\n"
    "- **trigger_preview()**: Renders the current editor code in the 3D viewport. "
    "Automatically postponed if code changes are pending review.";
  ai_history.push_back({"system", ref_doc});

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

  auto on_complete_wrapper = [this, config, &history, on_chunk, on_error, on_complete,
                              sys_prompt](const std::vector<AIToolCall>& tool_calls) {
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
      t["id"] = tc.id;
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
      tool_msg.tool_call_id = tc.id;
      history.push_back(tool_msg);
    }

    chatCompletionStream(history, on_chunk, on_error, on_complete);
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
    "   - YOU MUST USE `set_editor_code` to propose any code changes so they are set for user review.\n"
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
    ai_history.push_back({"system", sys_prompt});
  }

  for (const auto& msg : history) {
    AIChatMessage am;
    am.role = msg.role;
    am.content = msg.content;
    am.tool_call_id = msg.tool_call_id;
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
                                     CompleteCallback)
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

#endif  // __EMSCRIPTEN__
