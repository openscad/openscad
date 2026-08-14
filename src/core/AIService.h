#ifndef OPENSCAD_AISERVICE_H
#define OPENSCAD_AISERVICE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

struct ImageAttachment {
  std::string mime_type;    // e.g. "image/png"
  std::string base64_data;  // raw base64 string
};

struct ChatMessage {
  std::string role;
  std::string content;
  std::string tool_call_id;
  std::string tool_calls;  // Serialized JSON representation
  std::vector<ImageAttachment> images;
};

class AIService
{
public:
  using ChunkCallback = std::function<void(const std::string& chunk)>;
  using ResponseCallback = std::function<void(const std::string& full_response)>;
  using ErrorCallback = std::function<void(const std::string& error_msg)>;
  using CompleteCallback = std::function<void()>;

  AIService();
  ~AIService();

  // Prevent copy, allow move
  AIService(const AIService&) = delete;
  AIService& operator=(const AIService&) = delete;
  AIService(AIService&&) noexcept;
  AIService& operator=(AIService&&) noexcept;

  // Async chat completion with streaming output
  void chatCompletionStream(std::vector<ChatMessage>& history, ChunkCallback on_chunk,
                            ErrorCallback on_error, CompleteCallback on_complete,
                            bool interactiveMode = false, int current_auto_turn = 0);

  // Async chat completion with full non-streaming response
  void chatCompletion(const std::vector<ChatMessage>& history, ResponseCallback on_response,
                      ErrorCallback on_error);

  // Get default user prompt configured in the active profile settings
  std::string getDefaultPrompt() const;

  // Cancel any active AI completion requests
  void cancelPendingRequests();

  // Get the configured character/payload size limit
  int getPayloadLimit() const;

  // Get auto attach viewport setting
  bool getAutoAttachViewport() const;

  // Get max auto turns setting
  int getMaxAutoTurns() const;

  using ToolExecutor =
    std::function<std::string(const std::string& name, const std::string& arguments_json)>;
  void registerToolExecutor(ToolExecutor executor);

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

#endif  // OPENSCAD_AISERVICE_H
