#include "AIProvider.h"
#include "AIClient.h"
#include <algorithm>

namespace {
AIProviderRequest baseRequest(const AIProfileConfig& config, const std::string& path,
                              const std::string& authHeader, const std::string& authPrefix,
                              const nlohmann::json& payload)
{
  std::string base = config.endpoint;
  while (!base.empty() && base.back() == '/') base.pop_back();
  AIProviderRequest request;
  request.url = base + path;
  request.headers["Content-Type"] = "application/json";
  if (!config.apiKey.empty()) request.headers[authHeader] = authPrefix + config.apiKey;
  request.body = payload.dump();
  return request;
}

std::string responseTextParts(const nlohmann::json& array, const std::string& textKey)
{
  if (!array.is_array()) return {};
  std::string text;
  for (const auto& item : array) {
    if (item.is_object() && item.contains(textKey) && item[textKey].is_string())
      text += item[textKey].get<std::string>();
  }
  return text;
}
}

AIProviderRequest OpenAIProvider::makeCodeRequest(const AIProfileConfig& config,
                                                   const std::string& system,
                                                   const std::string& prompt, bool stream) const
{
  nlohmann::json payload = {
    {"model", config.model}, {"instructions", system}, {"input", prompt}, {"stream", stream}
  };
  if (config.parameters.contains("max_tokens")) payload["max_output_tokens"] = config.parameters["max_tokens"];
  return baseRequest(config, "/responses", "Authorization", "Bearer ", payload);
}

std::string OpenAIProvider::parseResponse(const nlohmann::json& response) const
{
  if (response.contains("output_text") && response["output_text"].is_string())
    return response["output_text"].get<std::string>();
  if (!response.contains("output") || !response["output"].is_array()) return {};
  std::string text;
  for (const auto& item : response["output"]) {
    if (!item.contains("content") || !item["content"].is_array()) continue;
    for (const auto& content : item["content"]) {
      if (content.value("type", std::string{}) == "output_text")
        text += content.value("text", std::string{});
    }
  }
  return text;
}

std::string OpenAIProvider::parseStreamEvent(const nlohmann::json& event) const
{
  if (event.value("type", std::string{}) == "response.output_text.delta")
    return event.value("delta", std::string{});
  if (event.contains("error")) return {};
  return {};
}

AIProviderRequest GeminiProvider::makeCodeRequest(const AIProfileConfig& config,
                                                   const std::string& system,
                                                   const std::string& prompt, bool stream) const
{
  std::string model = config.model.empty() ? AIProviderManager::defaultModel("Google Gemini") : config.model;
  nlohmann::json payload = {
    {"systemInstruction", {{"parts", nlohmann::json::array({{{"text", system}}})}}},
    {"contents", nlohmann::json::array({{{"role", "user"}, {"parts", nlohmann::json::array({{{"text", prompt}}})}}})},
    {"generationConfig", {{"maxOutputTokens", config.parameters.value("max_tokens", 4096)}}}
  };
  const std::string path = "/models/" + model + (stream ? ":streamGenerateContent?alt=sse" : ":generateContent");
  return baseRequest(config, path, "x-goog-api-key", "", payload);
}

std::string GeminiProvider::parseResponse(const nlohmann::json& response) const
{
  if (!response.contains("candidates") || !response["candidates"].is_array() || response["candidates"].empty())
    return {};
  const auto& content = response["candidates"][0].value("content", nlohmann::json::object());
  return content.contains("parts") ? responseTextParts(content["parts"], "text") : std::string{};
}

std::string GeminiProvider::parseStreamEvent(const nlohmann::json& event) const
{
  return parseResponse(event);
}

AIProviderRequest ClaudeProvider::makeCodeRequest(const AIProfileConfig& config,
                                                   const std::string& system,
                                                   const std::string& prompt, bool stream) const
{
  nlohmann::json payload = {
    {"model", config.model}, {"max_tokens", config.parameters.value("max_tokens", 4096)},
    {"system", system}, {"messages", nlohmann::json::array({{{"role", "user"}, {"content", prompt}}})},
    {"stream", stream}
  };
  AIProviderRequest request = baseRequest(config, "/messages", "x-api-key", "", payload);
  request.headers["anthropic-version"] = "2023-06-01";
  return request;
}

std::string ClaudeProvider::parseResponse(const nlohmann::json& response) const
{
  return response.contains("content") ? responseTextParts(response["content"], "text") : std::string{};
}

std::string ClaudeProvider::parseStreamEvent(const nlohmann::json& event) const
{
  if (event.value("type", std::string{}) != "content_block_delta" ||
      !event.contains("delta") || !event["delta"].is_object()) return {};
  const auto& delta = event["delta"];
  if (delta.value("type", std::string{}) == "text_delta") return delta.value("text", std::string{});
  return {};
}

std::shared_ptr<const AIProvider> AIProviderManager::forProfile(const std::string& profile)
{
  if (profile.find("Gemini") != std::string::npos || profile.find("Google") != std::string::npos)
    return std::make_shared<GeminiProvider>();
  if (profile.find("Claude") != std::string::npos || profile.find("Anthropic") != std::string::npos)
    return std::make_shared<ClaudeProvider>();
  return std::make_shared<OpenAIProvider>();
}

std::string AIProviderManager::defaultEndpoint(const std::string& profile)
{
  if (forProfile(profile)->name() == "Gemini") return "https://generativelanguage.googleapis.com/v1beta";
  if (forProfile(profile)->name() == "Claude") return "https://api.anthropic.com/v1";
  return "https://api.openai.com/v1";
}

std::string AIProviderManager::defaultModel(const std::string& profile)
{
  if (forProfile(profile)->name() == "Gemini") return "gemini-3.8-flash";
  if (forProfile(profile)->name() == "Claude") return "claude-sonnet-5-5";
  return "gpt-6.2-sol";
}
