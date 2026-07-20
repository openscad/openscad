#include "AIClient.h"
#include "HTTPClient.h"
#include <cctype>
#include <sstream>

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
    if (line.rfind("data: ", 0) == 0) {
      content = line.substr(6);
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
          if (delta.contains("content")) {
            std::string text = delta["content"].get<std::string>();
            if (!text.empty()) {
              callback_(text);
              matched = true;
            }
          }
          if (delta.contains("reasoning_content")) {
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

  return tools;
}

void AIClient::sendChatCompletion(const AIProfileConfig& config,
                                  const std::vector<AIChatMessage>& history,
                                  ResponseCallback on_response, ErrorCallback on_error)
{
  nlohmann::json payload = nlohmann::json::object();
  payload["model"] = config.model;
  payload["stream"] = false;

  nlohmann::json messages = nlohmann::json::array();
  for (const auto& msg : history) {
    nlohmann::json m = nlohmann::json::object();
    m["role"] = msg.role;
    if (!msg.content.empty() || msg.tool_calls.empty()) {
      m["content"] = msg.content;
    } else {
      m["content"] = nullptr;
    }
    if (msg.role == "tool") {
      m["tool_call_id"] = msg.tool_call_id;
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

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages" || key == "tools") {
        continue;
      }
      payload[key] = el.value();
    }
  }

  std::string body = payload.dump();

  HTTPClient::Headers headers;
  headers["Content-Type"] = "application/json";
  if (!config.apiKey.empty()) {
    headers["Authorization"] = "Bearer " + config.apiKey;
  }

  std::string endpoint_url = config.endpoint;
  if (endpoint_url.find("/chat/completions") == std::string::npos &&
      endpoint_url.find("/generate") == std::string::npos &&
      endpoint_url.find("/api/chat") == std::string::npos) {
    if (!endpoint_url.empty() && endpoint_url.back() == '/') {
      endpoint_url += "chat/completions";
    } else {
      endpoint_url += "/chat/completions";
    }
  }

  impl->http_client->asyncPost(
    endpoint_url, headers, body,
    [on_response, on_error](int status_code, const std::string& response_body) {
      if (status_code >= 200 && status_code < 300) {
        try {
          auto json = nlohmann::json::parse(response_body);
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

void AIClient::sendChatCompletionStream(const AIProfileConfig& config,
                                        const std::vector<AIChatMessage>& history,
                                        ChunkCallback on_chunk, ErrorCallback on_error,
                                        CompleteCallback on_complete)
{
  nlohmann::json payload = nlohmann::json::object();
  payload["model"] = config.model;
  payload["stream"] = true;

  nlohmann::json messages = nlohmann::json::array();
  for (const auto& msg : history) {
    nlohmann::json m = nlohmann::json::object();
    m["role"] = msg.role;
    if (!msg.content.empty() || msg.tool_calls.empty()) {
      m["content"] = msg.content;
    } else {
      m["content"] = nullptr;
    }
    if (msg.role == "tool") {
      m["tool_call_id"] = msg.tool_call_id;
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

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages" || key == "tools") {
        continue;
      }
      payload[key] = el.value();
    }
  }

  std::string body = payload.dump();

  HTTPClient::Headers headers;
  headers["Content-Type"] = "application/json";
  if (!config.apiKey.empty()) {
    headers["Authorization"] = "Bearer " + config.apiKey;
  }

  std::string endpoint_url = config.endpoint;
  if (endpoint_url.find("/chat/completions") == std::string::npos &&
      endpoint_url.find("/generate") == std::string::npos &&
      endpoint_url.find("/api/chat") == std::string::npos) {
    if (!endpoint_url.empty() && endpoint_url.back() == '/') {
      endpoint_url += "chat/completions";
    } else {
      endpoint_url += "/chat/completions";
    }
  }

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
