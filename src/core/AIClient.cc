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

private:
  std::string buffer_;
  TextCallback callback_;

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
    m["content"] = msg.content;
    messages.push_back(m);
  }
  payload["messages"] = messages;

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages") {
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
            if (choice.contains("message") && choice["message"].is_object() &&
                choice["message"].contains("content")) {
              on_response(choice["message"]["content"].get<std::string>());
              return;
            }
          }
          if (json.contains("message") && json["message"].is_object() &&
              json["message"].contains("content")) {
            on_response(json["message"]["content"].get<std::string>());
            return;
          }
          if (json.contains("response")) {
            on_response(json["response"].get<std::string>());
            return;
          }
          on_response(response_body);
        } catch (...) {
          on_response(response_body);
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
    m["content"] = msg.content;
    messages.push_back(m);
  }
  payload["messages"] = messages;

  if (config.parameters.is_object()) {
    for (auto& el : config.parameters.items()) {
      const std::string& key = el.key();
      if (key == "model" || key == "stream" || key == "messages") {
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
        on_complete();
      }
    });
}
