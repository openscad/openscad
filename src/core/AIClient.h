#ifndef OPENSCAD_AICLIENT_H
#define OPENSCAD_AICLIENT_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "json/json.hpp"

struct AIChatMessage {
  std::string role;
  std::string content;
};

struct AIProfileConfig {
  std::string endpoint;
  std::string apiKey;
  std::string model;
  nlohmann::json parameters;
};

class HTTPClient;

class AIClient
{
public:
  using ChunkCallback = std::function<void(const std::string& chunk)>;
  using ResponseCallback = std::function<void(const std::string& response)>;
  using ErrorCallback = std::function<void(const std::string& error_msg)>;
  using CompleteCallback = std::function<void()>;

  AIClient(std::shared_ptr<HTTPClient> httpClient);
  ~AIClient();

  // Prevent copy, allow move
  AIClient(const AIClient&) = delete;
  AIClient& operator=(const AIClient&) = delete;
  AIClient(AIClient&&) noexcept;
  AIClient& operator=(AIClient&&) noexcept;

  // Asynchronous OpenAI-compatible POST request
  void sendChatCompletion(const AIProfileConfig& config, const std::vector<AIChatMessage>& history,
                          ResponseCallback on_response, ErrorCallback on_error);

  // Asynchronous OpenAI-compatible POST request with streaming response
  void sendChatCompletionStream(const AIProfileConfig& config, const std::vector<AIChatMessage>& history,
                                ChunkCallback on_chunk, ErrorCallback on_error,
                                CompleteCallback on_complete);

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

#endif  // OPENSCAD_AICLIENT_H
