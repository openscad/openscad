#include "AIService.h"

#ifndef __EMSCRIPTEN__

#include "HTTPClient.h"
#include "AIClient.h"
#include "platform/PlatformUtils.h"
#include "json/json.hpp"
#include <fstream>
#include <cstdlib>
#include <future>
#include <QApplication>
#include "gui/MainWindow.h"
#include "gui/OpenSCADApp.h"

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

static std::string executeToolOnMainThread(const std::string& name, const std::string& arguments_json)
{
  auto promise = std::make_shared<std::promise<std::string>>();
  auto future = promise->get_future();

  QMetaObject::invokeMethod(
    qApp,
    [promise, name, arguments_json]() {
      try {
        MainWindow *mw = nullptr;
        for (auto *win : scadApp->windowManager.getWindows()) {
          mw = win;
          break;
        }

        if (name == "get_editor_code") {
          if (mw && mw->activeEditor) {
            std::string code = mw->activeEditor->toPlainText().toStdString();
            promise->set_value(code);
          } else {
            promise->set_value("Error: No active editor found.");
          }
        } else if (name == "set_editor_code") {
          auto args = nlohmann::json::parse(arguments_json);
          if (!args.contains("code")) {
            promise->set_value("Error: Missing required argument 'code'.");
            return;
          }
          std::string code = args["code"].get<std::string>();
          if (mw && mw->activeEditor) {
            mw->activeEditor->setText(QString::fromStdString(code));
            promise->set_value("Success: Code set in the editor.");
          } else {
            promise->set_value("Error: No active editor found.");
          }
        } else if (name == "trigger_preview") {
          if (mw) {
            mw->actionRenderPreview();
            promise->set_value("Success: Preview triggered.");
          } else {
            promise->set_value("Error: No active MainWindow found.");
          }
        } else {
          promise->set_value("Error: Unknown tool name '" + name + "'.");
        }
      } catch (const std::exception& e) {
        promise->set_value(std::string("Error parsing/executing tool: ") + e.what());
      } catch (...) {
        promise->set_value("Error: Unknown exception occurred during tool execution.");
      }
    },
    Qt::QueuedConnection);

  return future.get();
}

class AIService::Impl
{
public:
  std::unique_ptr<HTTPClient> http_client;
  std::unique_ptr<AIClient> ai_client;

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

  auto on_complete_wrapper = [this, config, &history, on_chunk, on_error, on_complete,
                              sys_prompt](const std::vector<AIToolCall>& tool_calls) {
    if (tool_calls.empty()) {
      if (on_complete) {
        on_complete();
      }
      return;
    }

    std::string tool_indicator = "\n\n*[Executing assistant tools...]*\n";
    on_chunk(tool_indicator);

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
      std::string result = executeToolOnMainThread(tc.name, tc.arguments);
      ChatMessage tool_msg;
      tool_msg.role = "tool";
      tool_msg.content = result;
      tool_msg.tool_call_id = tc.id;
      history.push_back(tool_msg);

      on_chunk("- **" + tc.name + "**: " + result + "\n");
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

#endif  // __EMSCRIPTEN__
