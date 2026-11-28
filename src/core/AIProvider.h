#ifndef OPENSCAD_AIPROVIDER_H
#define OPENSCAD_AIPROVIDER_H

#include <memory>
#include <string>
#include <map>
#include "json/json.hpp"

struct AIProfileConfig;

struct AIProviderRequest {
  std::string url;
  std::map<std::string, std::string> headers;
  std::string body;
};

class AIProvider
{
public:
  virtual ~AIProvider() = default;
  virtual AIProviderRequest makeCodeRequest(const AIProfileConfig& config,
                                            const std::string& systemPrompt,
                                            const std::string& userPrompt, bool stream) const = 0;
  virtual std::string parseResponse(const nlohmann::json& response) const = 0;
  virtual std::string parseStreamEvent(const nlohmann::json& event) const = 0;
  virtual std::string name() const = 0;
};

class OpenAIProvider final : public AIProvider
{
public:
  AIProviderRequest makeCodeRequest(const AIProfileConfig&, const std::string&, const std::string&, bool) const override;
  std::string parseResponse(const nlohmann::json&) const override;
  std::string parseStreamEvent(const nlohmann::json&) const override;
  std::string name() const override { return "OpenAI"; }
};

class GeminiProvider final : public AIProvider
{
public:
  AIProviderRequest makeCodeRequest(const AIProfileConfig&, const std::string&, const std::string&, bool) const override;
  std::string parseResponse(const nlohmann::json&) const override;
  std::string parseStreamEvent(const nlohmann::json&) const override;
  std::string name() const override { return "Gemini"; }
};

class ClaudeProvider final : public AIProvider
{
public:
  AIProviderRequest makeCodeRequest(const AIProfileConfig&, const std::string&, const std::string&, bool) const override;
  std::string parseResponse(const nlohmann::json&) const override;
  std::string parseStreamEvent(const nlohmann::json&) const override;
  std::string name() const override { return "Claude"; }
};

class AIProviderManager
{
public:
  static std::shared_ptr<const AIProvider> forProfile(const std::string& profileName);
  static std::string defaultEndpoint(const std::string& profileName);
  static std::string defaultModel(const std::string& profileName);
};

#endif
