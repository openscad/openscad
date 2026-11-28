#include "AIClient.h"
#include "HTTPClient.h"
#include "AIProvider.h"
#include <algorithm>
#include <cctype>
#include <sstream>

class CodeSSEParser
{
public:
  CodeSSEParser(std::shared_ptr<const AIProvider> provider, AIClient::ChunkCallback callback)
    : provider_(std::move(provider)), callback_(std::move(callback)) {}

  void feed(const std::string& data)
  {
    buffer_ += data;
    size_t newline;
    while ((newline = buffer_.find('\n')) != std::string::npos) {
      std::string line = buffer_.substr(0, newline);
      buffer_.erase(0, newline + 1);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.rfind("data:", 0) != 0) continue;
      std::string payload = line.substr(5);
      while (!payload.empty() && std::isspace(static_cast<unsigned char>(payload.front()))) payload.erase(payload.begin());
      if (payload.empty() || payload == "[DONE]") continue;
      try {
        std::string text = provider_->parseStreamEvent(nlohmann::json::parse(payload));
        if (!text.empty() && callback_) callback_(text);
      } catch (...) {
      }
    }
  }

  void flush() { feed("\n"); }

private:
  std::shared_ptr<const AIProvider> provider_;
  AIClient::ChunkCallback callback_;
  std::string buffer_;
};

static std::string apiErrorMessage(int status, const std::string& body)
{
  std::string detail = body;
  try {
    const auto json = nlohmann::json::parse(body);
    if (json.contains("error")) {
      const auto& error = json["error"];
      if (error.is_object()) {
        detail = error.value("message", error.value("status", body));
      } else if (error.is_string()) {
        detail = error.get<std::string>();
      }
    }
  } catch (...) {
  }
  std::string lower = detail;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (status == 401) return "API key was rejected. Check the key for this provider.";
  if (status == 403) return "The key cannot access this model or project. Check provider access and billing. " + detail;
  if (status == 404) return "The model or API endpoint was not found. Choose another model or review Advanced Settings. " + detail;
  if (status == 429 || lower.find("quota") != std::string::npos || lower.find("billing") != std::string::npos ||
      lower.find("credit") != std::string::npos) return "Provider quota, credits, or billing limit reached. " + detail;
  return "HTTP status " + std::to_string(status) + ": " + detail;
}

class SSEParser
{
public:
  using TextCallback = std::function<void(const std::string& text)>;

  SSEParser(TextCallback callback) : callback_(std::move(callback)) {}

  void feed(const std::string& data)
  {
    buffer_ += data;
    size_t newline;
    while ((newline = buffer_.find('\n')) != std::string::npos) {
      std::string line = buffer_.substr(0, newline);
      buffer_ = buffer_.substr(newline + 1);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      processLine(line);
    }
  }

  void flush()
  {
    if (!buffer_.empty()) {
      processLine(buffer_);
      buffer_.clear();
    }
  }

  const std::vector<AIToolCall>& toolCalls() const { return tool_calls_; }

private:
  std::string buffer_;
  TextCallback callback_;
  std::vector<AIToolCall> tool_calls_;

  void processLine(const std::string& line)
  {
    std::string content = line;
    if (line.rfind("data:", 0) == 0) {
      content = line.substr(5);
    }

    while (!content.empty() && std::isspace(static_cast<unsigned char>(content.front()))) {
      content.erase(0, 1);
    }
    while (!content.empty() && std::isspace(static_cast<unsigned char>(content.back()))) {
      content.pop_back();
    }

    if (content.empty()) {
      return;
    }

    if (content == "[DONE]") {
      return;
    }

    try {
      auto json = nlohmann::json::parse(content);
      bool matched = false;

      // OpenAI-compatible /v1/chat/completions format (content and reasoning_content)
      if (json.contains("choices") && json["choices"].is_array() && !json["choices"].empty()) {
        auto& choice = json["choices"][0];
        if (choice.contains("delta") && choice["delta"].is_object()) {
          auto& delta = choice["delta"];
          if (delta.contains("content") && delta["content"].is_string()) {
            std::string text = delta["content"].get<std::string>();
            if (!text.empty()) {
              callback_(text);
              matched = true;
            }
          }
          if (delta.contains("reasoning_content") && delta["reasoning_content"].is_string()) {
            std::string text = delta["reasoning_content"].get<std::string>();
            if (!text.empty()) {
              callback_(text);
              matched = true;
            }
          }
          if (delta.contains("tool_calls") && delta["tool_calls"].is_array()) {
            for (auto& tc : delta["tool_calls"]) {
              if (tc.is_object()) {
                size_t index = tc.value("index", 0);
                if (tool_calls_.size() <= index) {
                  tool_calls_.resize(index + 1);
                }
                auto& dest = tool_calls_[index];
                if (tc.contains("id")) {
                  dest.id = tc["id"].get<std::string>();
                }
                if (tc.contains("type")) {
                  dest.type = tc["type"].get<std::string>();
                }
                if (tc.contains("function") && tc["function"].is_object()) {
                  auto& fn = tc["function"];
                  if (fn.contains("name")) {
                    dest.name = fn["name"].get<std::string>();
                  }
                  if (fn.contains("arguments")) {
                    dest.arguments += fn["arguments"].get<std::string>();
                  }
                }
              }
            }
          }
        }
      }

      // Ollama /api/chat format (content and thinking)
      if (!matched && json.contains("message") && json["message"].is_object()) {
        auto& message = json["message"];
        if (message.contains("content")) {
          std::string text = message["content"].get<std::string>();
          if (!text.empty()) {
            callback_(text);
            matched = true;
          }
        }
        if (message.contains("thinking")) {
          std::string text = message["thinking"].get<std::string>();
          if (!text.empty()) {
            callback_(text);
            matched = true;
          }
        }
        if (message.contains("tool_calls") && message["tool_calls"].is_array()) {
          for (auto& tc : message["tool_calls"]) {
            AIToolCall tool_call;
            if (tc.contains("id")) {
              tool_call.id = tc["id"].get<std::string>();
            }
            if (tc.contains("type")) {
              tool_call.type = tc["type"].get<std::string>();
            }
            if (tc.contains("function") && tc["function"].is_object()) {
              auto& fn = tc["function"];
              if (fn.contains("name")) {
                tool_call.name = fn["name"].get<std::string>();
              }
              if (fn.contains("arguments")) {
                if (fn["arguments"].is_string()) {
                  tool_call.arguments = fn["arguments"].get<std::string>();
                } else {
                  tool_call.arguments = fn["arguments"].dump();
                }
              }
            }
            tool_calls_.push_back(tool_call);
          }
        }
      }

      // Ollama /api/generate response format
      if (!matched && json.contains("response")) {
        std::string text = json["response"].get<std::string>();
        if (!text.empty()) {
          callback_(text);
          matched = true;
        }
      }

      // Ollama /api/generate thinking format
      if (!matched && json.contains("thinking")) {
        std::string text = json["thinking"].get<std::string>();
        if (!text.empty()) {
          callback_(text);
          matched = true;
        }
      }

      // Anthropic /v1/messages streaming format
      if (!matched && json.contains("type") && json["type"].is_string()) {
        std::string type = json["type"].get<std::string>();
        if (type == "content_block_start" && json.contains("content_block") && json["content_block"].is_object()) {
          auto& cb = json["content_block"];
          if (cb.value("type", "") == "tool_use") {
            AIToolCall tc;
            tc.id = cb.value("id", "");
            tc.name = cb.value("name", "");
            tc.type = "function";
            size_t index = json.value("index", 0);
            if (tool_calls_.size() <= index) {
              tool_calls_.resize(index + 1);
            }
            tool_calls_[index] = tc;
            matched = true;
          }
        } else if (type == "content_block_delta" && json.contains("delta") && json["delta"].is_object()) {
          auto& delta = json["delta"];
          std::string delta_type = delta.value("type", "");
          if (delta_type == "text_delta" && delta.contains("text")) {
            std::string text = delta["text"].get<std::string>();
            if (!text.empty()) {
              callback_(text);
              matched = true;
            }
          } else if (delta_type == "input_json_delta" && delta.contains("partial_json")) {
            size_t index = json.value("index", 0);
            if (tool_calls_.size() > index) {
              tool_calls_[index].arguments += delta["partial_json"].get<std::string>();
              matched = true;
            }
          }
        }
      }
    } catch (...) {
      // Ignore parsing errors on comments or partial/malformed JSON
    }
  }
};

class AIClient::Impl
{
public:
  std::shared_ptr<HTTPClient> http_client;

  Impl(std::shared_ptr<HTTPClient> client) : http_client(std::move(client)) {}
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;
};

AIClient::AIClient(std::shared_ptr<HTTPClient> httpClient)
  : impl(std::make_unique<Impl>(std::move(httpClient)))
{
}

AIClient::~AIClient() = default;

AIClient::AIClient(AIClient&&) noexcept = default;
AIClient& AIClient::operator=(AIClient&&) noexcept = default;

nlohmann::json getOpenSCADTools()
{
  nlohmann::json tools = nlohmann::json::array();

  nlohmann::json set_editor_code = nlohmann::json::object();
  set_editor_code["type"] = "function";
  nlohmann::json sec_fn = nlohmann::json::object();
  sec_fn["name"] = "set_editor_code";
  sec_fn["description"] =
    "Propose changes to the current script in the editor. Use this to update the script.";
  nlohmann::json sec_params = nlohmann::json::object();
  sec_params["type"] = "object";
  nlohmann::json sec_props = nlohmann::json::object();
  nlohmann::json sec_code = nlohmann::json::object();
  sec_code["type"] = "string";
  sec_code["description"] = "The complete new code or content to put in the editor.";
  sec_props["code"] = sec_code;
  sec_params["properties"] = sec_props;
  sec_params["required"] = nlohmann::json::array({"code"});
  sec_fn["parameters"] = sec_params;
  set_editor_code["function"] = sec_fn;
  tools.push_back(set_editor_code);

  nlohmann::json get_editor_code = nlohmann::json::object();
  get_editor_code["type"] = "function";
  nlohmann::json gec_fn = nlohmann::json::object();
  gec_fn["name"] = "get_editor_code";
  gec_fn["description"] = "Retrieve the current source code present in the editor to inspect it.";
  nlohmann::json gec_params = nlohmann::json::object();
  gec_params["type"] = "object";
  gec_params["properties"] = nlohmann::json::object();
  gec_fn["parameters"] = gec_params;
  get_editor_code["function"] = gec_fn;
  tools.push_back(get_editor_code);

  nlohmann::json trigger_preview = nlohmann::json::object();
  trigger_preview["type"] = "function";
  nlohmann::json tp_fn = nlohmann::json::object();
  tp_fn["name"] = "trigger_preview";
  tp_fn["description"] =
    "Compile the current script and render a preview in the 3D viewport to validate the result.";
  nlohmann::json tp_params = nlohmann::json::object();
  tp_params["type"] = "object";
  tp_params["properties"] = nlohmann::json::object();
  tp_fn["parameters"] = tp_params;
  trigger_preview["function"] = tp_fn;
  tools.push_back(trigger_preview);

  nlohmann::json get_viewport = nlohmann::json::object();
  get_viewport["type"] = "function";
  nlohmann::json gv_fn = nlohmann::json::object();
  gv_fn["name"] = "get_viewport";
  gv_fn["description"] =
    "Retrieve the current 3D viewport camera parameters (vpt, vpr, vpd, vpf, projection).";
  nlohmann::json gv_params = nlohmann::json::object();
  gv_params["type"] = "object";
  gv_params["properties"] = nlohmann::json::object();
  gv_fn["parameters"] = gv_params;
  get_viewport["function"] = gv_fn;
  tools.push_back(get_viewport);

  nlohmann::json set_camera = nlohmann::json::object();
  set_camera["type"] = "function";
  nlohmann::json sc_fn = nlohmann::json::object();
  sc_fn["name"] = "set_camera";
  sc_fn["description"] =
    "Set the 3D viewport camera translation (vpt), rotation (vpr), distance (vpd), field of view (vpf), "
    "or projection mode.";
  nlohmann::json sc_params = nlohmann::json::object();
  sc_params["type"] = "object";
  nlohmann::json sc_props = nlohmann::json::object();

  nlohmann::json vpt_arr = nlohmann::json::object();
  vpt_arr["type"] = "array";
  vpt_arr["description"] = "Translation vector [x, y, z] (vpt).";
  vpt_arr["items"] = nlohmann::json::object({{"type", "number"}});
  sc_props["vpt"] = vpt_arr;

  nlohmann::json vpr_arr = nlohmann::json::object();
  vpr_arr["type"] = "array";
  vpr_arr["description"] = "Rotation angles in degrees [x, y, z] (vpr).";
  vpr_arr["items"] = nlohmann::json::object({{"type", "number"}});
  sc_props["vpr"] = vpr_arr;

  nlohmann::json vpd_num = nlohmann::json::object();
  vpd_num["type"] = "number";
  vpd_num["description"] = "Camera distance (vpd).";
  sc_props["vpd"] = vpd_num;

  nlohmann::json vpf_num = nlohmann::json::object();
  vpf_num["type"] = "number";
  vpf_num["description"] = "Field of view angle (vpf).";
  sc_props["vpf"] = vpf_num;

  nlohmann::json proj_str = nlohmann::json::object();
  proj_str["type"] = "string";
  proj_str["description"] = "Projection mode: 'ortho' / 'orthogonal' or 'perspective'.";
  sc_props["projection"] = proj_str;

  sc_params["properties"] = sc_props;
  sc_fn["parameters"] = sc_params;
  set_camera["function"] = sc_fn;
  tools.push_back(set_camera);

  return tools;
}

nlohmann::json getAnthropicTools()
{
  nlohmann::json openaiTools = getOpenSCADTools();
  nlohmann::json anthropicTools = nlohmann::json::array();
  for (const auto& t : openaiTools) {
    if (t.contains("function") && t["function"].is_object()) {
      const auto& fn = t["function"];
      nlohmann::json at = nlohmann::json::object();
      at["name"] = fn.value("name", "");
      at["description"] = fn.value("description", "");
      at["input_schema"] = fn.value("parameters", nlohmann::json::object());
      anthropicTools.push_back(at);
    }
  }
  return anthropicTools;
}

static std::string resolveEndpointUrl(const std::string& endpoint, bool isAnthropic)
{
  std::string url = endpoint;
  if (isAnthropic) {
    if (url.find("/messages") == std::string::npos) {
      if (!url.empty() && url.back() == '/') {
        url += "messages";
      } else {
        url += "/messages";
      }
    }
  } else if (url.find("googleapis.com") != std::string::npos) {
    if (url.find("/chat/completions") == std::string::npos) {
      if (url.find("/openai") == std::string::npos) {
        if (!url.empty() && url.back() == '/') {
          url += "openai/chat/completions";
        } else {
          url += "/openai/chat/completions";
        }
      } else {
        if (!url.empty() && url.back() == '/') {
          url += "chat/completions";
        } else {
          url += "/chat/completions";
        }
      }
    }
  } else if (url.find("/chat/completions") == std::string::npos &&
             url.find("/generate") == std::string::npos &&
             url.find("/api/chat") == std::string::npos) {
    if (!url.empty() && url.back() == '/') {
      url += "chat/completions";
    } else {
      url += "/chat/completions";
    }
  }
  return url;
}

static nlohmann::json buildAnthropicPayload(const AIProfileConfig& config,
                                            const std::vector<AIChatMessage>& history,
                                            bool stream)
{
  nlohmann::json payload = nlohmann::json::object();
  payload["model"] = config.model.empty() ? "claude-3-7-sonnet-20250219" : config.model;
  payload["max_tokens"] = 4096;
  payload["stream"] = stream;

  std::string sys_prompt = "";
  nlohmann::json messages = nlohmann::json::array();

  for (const auto& msg : history) {
    if (msg.role == "system") {
      if (!sys_prompt.empty()) sys_prompt += "\n\n";
      sys_prompt += msg.content;
      continue;
    }

    nlohmann::json m = nlohmann::json::object();
    if (msg.role == "tool") {
      m["role"] = "user";
      nlohmann::json content_arr = nlohmann::json::array();
      nlohmann::json tr = nlohmann::json::object();
      tr["type"] = "tool_result";
      tr["tool_use_id"] = msg.tool_call_id;
      tr["content"] = msg.content;
      content_arr.push_back(tr);
      m["content"] = content_arr;
    } else if (!msg.tool_calls.empty()) {
      m["role"] = "assistant";
      nlohmann::json content_arr = nlohmann::json::array();
      if (!msg.content.empty()) {
        nlohmann::json txt = nlohmann::json::object();
        txt["type"] = "text";
        txt["text"] = msg.content;
        content_arr.push_back(txt);
      }
      for (const auto& tc : msg.tool_calls) {
        nlohmann::json tu = nlohmann::json::object();
        tu["type"] = "tool_use";
        tu["id"] = tc.id;
        tu["name"] = tc.name;
        try {
          tu["input"] = nlohmann::json::parse(tc.arguments);
        } catch (...) {
          tu["input"] = nlohmann::json::object();
        }
        content_arr.push_back(tu);
      }
      m["content"] = content_arr;
    } else {
      m["role"] = (msg.role == "assistant") ? "assistant" : "user";
      if (!msg.images.empty()) {
        nlohmann::json content_arr = nlohmann::json::array();
        if (!msg.content.empty()) {
          nlohmann::json txt = nlohmann::json::object();
          txt["type"] = "text";
          txt["text"] = msg.content;
          content_arr.push_back(txt);
        }
        for (const auto& img : msg.images) {
          nlohmann::json img_obj = nlohmann::json::object();
          img_obj["type"] = "image";
          nlohmann::json src = nlohmann::json::object();
          src["type"] = "base64";
          src["media_type"] = img.mime_type.empty() ? "image/png" : img.mime_type;
          src["data"] = img.base64_data;
          img_obj["source"] = src;
          content_arr.push_back(img_obj);
        }
        m["content"] = content_arr;
      } else {
        m["content"] = msg.content;
      }
    }
    messages.push_back(m);
  }

  if (!sys_prompt.empty()) {
    payload["system"] = sys_prompt;
  }
  payload["messages"] = messages;
  payload["tools"] = getAnthropicTools();

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages" || key == "tools" ||
          key == "system" || key == "system_prompt" || key == "default_prompt" ||
          key == "context_limit" || key == "payload_limit" || key == "auto_attach_viewport" ||
          key == "max_auto_turns" || key == "max_tokens") {
        continue;
      }
      payload[key] = el.value();
    }
    if (config.parameters.contains("max_tokens") && config.parameters["max_tokens"].is_number_integer()) {
      payload["max_tokens"] = config.parameters["max_tokens"].get<int>();
    }
  }

  return payload;
}

static nlohmann::json buildOpenAIPayload(const AIProfileConfig& config,
                                         const std::vector<AIChatMessage>& history,
                                         bool stream)
{
  nlohmann::json payload = nlohmann::json::object();
  payload["model"] = config.model;
  payload["stream"] = stream;

  nlohmann::json messages = nlohmann::json::array();
  for (const auto& msg : history) {
    nlohmann::json m = nlohmann::json::object();
    m["role"] = msg.role;
    if (!msg.images.empty()) {
      nlohmann::json content_arr = nlohmann::json::array();
      if (!msg.content.empty()) {
        nlohmann::json text_obj = nlohmann::json::object();
        text_obj["type"] = "text";
        text_obj["text"] = msg.content;
        content_arr.push_back(text_obj);
      }
      for (const auto& img : msg.images) {
        nlohmann::json img_obj = nlohmann::json::object();
        img_obj["type"] = "image_url";
        nlohmann::json url_obj = nlohmann::json::object();
        url_obj["url"] =
          "data:" + (img.mime_type.empty() ? "image/png" : img.mime_type) + ";base64," + img.base64_data;
        img_obj["image_url"] = url_obj;
        content_arr.push_back(img_obj);
      }
      m["content"] = content_arr;
    } else if (!msg.content.empty() || msg.tool_calls.empty()) {
      m["content"] = msg.content;
    } else {
      // Gemini's OpenAI-compatible endpoint rejects empty assistant content
      // on tool-call turns. Other OpenAI servers accept this marker as normal
      // assistant text accompanying the function call.
      m["content"] = "Calling OpenSCAD tools.";
    }
    if (msg.role == "tool") {
      m["tool_call_id"] = msg.tool_call_id;
      if (config.endpoint.find("generativelanguage.googleapis.com") != std::string::npos &&
          !msg.tool_name.empty()) {
        m["name"] = msg.tool_name;
      }
    }
    if (!msg.tool_calls.empty()) {
      nlohmann::json tcs = nlohmann::json::array();
      for (const auto& tc : msg.tool_calls) {
        nlohmann::json t = nlohmann::json::object();
        t["id"] = tc.id;
        t["type"] = "function";
        nlohmann::json fn = nlohmann::json::object();
        fn["name"] = tc.name;
        fn["arguments"] = tc.arguments;
        t["function"] = fn;
        tcs.push_back(t);
      }
      m["tool_calls"] = tcs;
    }
    messages.push_back(m);
  }
  payload["messages"] = messages;
  payload["tools"] = getOpenSCADTools();
  payload["tool_choice"] = "auto";
  // Gemini 3.8 Flash defaults to medium reasoning, which is excessive for
  // routine CAD edits and can make the chat appear idle before its first tool
  // call. Keep the request overridable from the provider profile.
  if (config.endpoint.find("generativelanguage.googleapis.com") != std::string::npos &&
      !config.parameters.contains("reasoning_effort")) {
    payload["reasoning_effort"] = "low";
  }

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages" || key == "tools" ||
          key == "system_prompt" || key == "default_prompt" || key == "context_limit" ||
          key == "payload_limit" || key == "auto_attach_viewport" || key == "max_auto_turns") {
        continue;
      }
      payload[key] = el.value();
    }
  }

  return payload;
}

void AIClient::sendChatCompletion(const AIProfileConfig& config,
                                  const std::vector<AIChatMessage>& history,
                                  ResponseCallback on_response, ErrorCallback on_error)
{
  bool isAnthropic = (config.endpoint.find("anthropic.com") != std::string::npos);
  nlohmann::json payload = isAnthropic ? buildAnthropicPayload(config, history, false)
                                       : buildOpenAIPayload(config, history, false);

  std::string body = payload.dump();

  HTTPClient::Headers headers;
  headers["Content-Type"] = "application/json";
  if (!config.apiKey.empty()) {
    if (isAnthropic) {
      headers["x-api-key"] = config.apiKey;
      headers["anthropic-version"] = "2023-06-01";
    } else {
      headers["Authorization"] = "Bearer " + config.apiKey;
    }
  }

  std::string endpoint_url = resolveEndpointUrl(config.endpoint, isAnthropic);

  impl->http_client->asyncPost(
    endpoint_url, headers, body,
    [on_response, on_error](int status_code, const std::string& response_body) {
      if (status_code >= 200 && status_code < 300) {
        try {
          auto json = nlohmann::json::parse(response_body);

          // Anthropic content array response
          if (json.contains("content") && json["content"].is_array()) {
            std::string content = "";
            std::vector<AIToolCall> parsed_tool_calls;
            for (const auto& item : json["content"]) {
              if (item.contains("type") && item["type"] == "text" && item.contains("text")) {
                content += item["text"].get<std::string>();
              } else if (item.contains("type") && item["type"] == "tool_use") {
                AIToolCall tc;
                tc.id = item.value("id", "");
                tc.name = item.value("name", "");
                tc.type = "function";
                if (item.contains("input")) {
                  if (item["input"].is_string()) {
                    tc.arguments = item["input"].get<std::string>();
                  } else {
                    tc.arguments = item["input"].dump();
                  }
                }
                parsed_tool_calls.push_back(tc);
              }
            }
            on_response(content, parsed_tool_calls);
            return;
          }

          if (json.contains("choices") && json["choices"].is_array() && !json["choices"].empty()) {
            auto& choice = json["choices"][0];
            std::string content = "";
            std::vector<AIToolCall> parsed_tool_calls;
            if (choice.contains("message") && choice["message"].is_object()) {
              auto& msg = choice["message"];
              if (msg.contains("content") && !msg["content"].is_null()) {
                content = msg["content"].get<std::string>();
              }
              if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
                for (auto& tc : msg["tool_calls"]) {
                  AIToolCall tool_call;
                  if (tc.contains("id")) {
                    tool_call.id = tc["id"].get<std::string>();
                  }
                  if (tc.contains("type")) {
                    tool_call.type = tc["type"].get<std::string>();
                  }
                  if (tc.contains("function") && tc["function"].is_object()) {
                    auto& fn = tc["function"];
                    if (fn.contains("name")) {
                      tool_call.name = fn["name"].get<std::string>();
                    }
                    if (fn.contains("arguments")) {
                      if (fn["arguments"].is_string()) {
                        tool_call.arguments = fn["arguments"].get<std::string>();
                      } else {
                        tool_call.arguments = fn["arguments"].dump();
                      }
                    }
                  }
                  parsed_tool_calls.push_back(tool_call);
                }
              }
            }
            on_response(content, parsed_tool_calls);
            return;
          }
          if (json.contains("message") && json["message"].is_object() &&
              json["message"].contains("content")) {
            on_response(json["message"]["content"].get<std::string>(), {});
            return;
          }
          if (json.contains("response")) {
            on_response(json["response"].get<std::string>(), {});
            return;
          }
          on_response(response_body, {});
        } catch (...) {
          on_response(response_body, {});
        }
      } else {
        if (on_error) {
          on_error("HTTP status " + std::to_string(status_code) + ": " + response_body);
        }
      }
    },
    on_error);
}

void AIClient::generateOpenSCADCodeStream(const AIProfileConfig& config,
                                          const std::string& systemPrompt,
                                          const std::string& userPrompt, ChunkCallback on_chunk,
                                          ErrorCallback on_error,
                                          std::function<void()> on_complete)
{
  const auto provider = AIProviderManager::forProfile(config.provider);
  const AIProviderRequest request = provider->makeCodeRequest(config, systemPrompt, userPrompt, true);
  auto parser = std::make_shared<CodeSSEParser>(provider, std::move(on_chunk));
  impl->http_client->asyncPostStream(
    request.url, request.headers, request.body,
    [parser, on_error](int status, const std::string& chunk) {
      if (status >= 200 && status < 300) {
        parser->feed(chunk);
      } else if (on_error) {
        on_error(apiErrorMessage(status, chunk));
      }
    }, on_error,
    [parser, on_complete]() {
      parser->flush();
      if (on_complete) on_complete();
    });
}

void AIClient::testConnection(const AIProfileConfig& config, std::function<void()> on_success,
                              ErrorCallback on_error)
{
  const auto provider = AIProviderManager::forProfile(config.provider);
  const AIProviderRequest request = provider->makeCodeRequest(
    config, "Return only the requested OpenSCAD source code.", "Return exactly this OpenSCAD code: cube(1);", false);
  impl->http_client->asyncPost(
    request.url, request.headers, request.body,
    [provider, on_success, on_error](int status, const std::string& body) {
      if (status < 200 || status >= 300) {
        if (on_error) on_error(apiErrorMessage(status, body));
        return;
      }
      try {
        const auto json = nlohmann::json::parse(body);
        const std::string text = provider->parseResponse(json);
        if (text.find("cube") == std::string::npos || text.find(';') == std::string::npos) {
          if (on_error) on_error("The API responded, but the selected model did not return the expected test code.");
          return;
        }
        if (on_success) on_success();
      } catch (const std::exception& e) {
        if (on_error) on_error(std::string("Could not validate provider response: ") + e.what());
      }
    },
    on_error);
}

void AIClient::sendChatCompletionStream(const AIProfileConfig& config,
                                        const std::vector<AIChatMessage>& history,
                                        ChunkCallback on_chunk, ErrorCallback on_error,
                                        CompleteCallback on_complete)
{
  bool isAnthropic = (config.endpoint.find("anthropic.com") != std::string::npos);
  nlohmann::json payload = isAnthropic ? buildAnthropicPayload(config, history, true)
                                       : buildOpenAIPayload(config, history, true);

  std::string body = payload.dump();

  HTTPClient::Headers headers;
  headers["Content-Type"] = "application/json";
  if (!config.apiKey.empty()) {
    if (isAnthropic) {
      headers["x-api-key"] = config.apiKey;
      headers["anthropic-version"] = "2023-06-01";
    } else {
      headers["Authorization"] = "Bearer " + config.apiKey;
    }
  }

  std::string endpoint_url = resolveEndpointUrl(config.endpoint, isAnthropic);

  auto parser = std::make_shared<SSEParser>(on_chunk);

  impl->http_client->asyncPostStream(
    endpoint_url, headers, body,
    [parser, on_error](int status_code, const std::string& chunk) {
      if (status_code >= 200 && status_code < 300) {
        parser->feed(chunk);
      } else {
        if (on_error) {
          on_error("HTTP status " + std::to_string(status_code) + ": " + chunk);
        }
      }
    },
    on_error,
    [parser, on_complete]() {
      parser->flush();
      if (on_complete) {
        on_complete(parser->toolCalls());
      }
    });
}

void AIClient::cancelPendingRequests()
{
  impl->http_client->cancelPendingRequests();
}
