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
  std::string tool_name;
};

struct AIProviderPreset {
  std::string name;          // e.g. "OpenAI ChatGPT", "Google Gemini", "Anthropic Claude", "Ollama Local"
  std::string endpoint;      // Default base endpoint URL
  std::string defaultModel;  // Default model identifier
  std::string envVar;        // Environment variable for API key
  std::string description;   // Human-readable summary
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

  // Provider presets and profile management
  static std::vector<AIProviderPreset> getProviderPresets();
  static std::vector<std::string> getAvailableProfileNames();
  static std::vector<std::string> getConfiguredProfileNames();
  static std::string getProfileModelName(const std::string& profileName);
  static std::string getActiveProfileName();
  static bool setActiveProfileName(const std::string& name);
  static bool createProfileFromPreset(const std::string& presetName, const std::string& apiKey = "");
  static void ensureDefaultProfiles();

  // Async chat completion with streaming output
  void chatCompletionStream(std::vector<ChatMessage>& history, ChunkCallback on_chunk,
                            ErrorCallback on_error, CompleteCallback on_complete,
                            bool interactiveMode = false, int current_auto_turn = 0);

  // Async chat completion with full non-streaming response
  void chatCompletion(const std::vector<ChatMessage>& history, ResponseCallback on_response,
                      ErrorCallback on_error);

  // Generate source code in one direct provider API request, with no MCP/tools.
  void generateOpenSCADCode(const std::string& prompt, const std::string& currentCode,
                            ChunkCallback on_chunk, ErrorCallback on_error,
                            CompleteCallback on_complete);

  void testConnection(CompleteCallback on_success, ErrorCallback on_error);

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

  // Optional callback invoked after each tool-execution round, before the next
  // recursive LLM call. The callback receives the history vector by reference
  // and may append additional messages (e.g. a viewport snapshot image message).
  using HistoryDrainCallback = std::function<void(std::vector<ChatMessage>& history)>;
  void registerHistoryDrainCallback(HistoryDrainCallback callback);

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

#endif  // OPENSCAD_AISERVICE_H
