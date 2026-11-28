#include "AIProfileManager.h"
#include <algorithm>
#include <iterator>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {
const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string toBase64(const unsigned char *data, size_t size)
{
  std::string out;
  for (size_t i = 0; i < size; i += 3) {
    unsigned int value = static_cast<unsigned int>(data[i]) << 16;
    if (i + 1 < size) value |= static_cast<unsigned int>(data[i + 1]) << 8;
    if (i + 2 < size) value |= static_cast<unsigned int>(data[i + 2]);
    out.push_back(alphabet[(value >> 18) & 63]);
    out.push_back(alphabet[(value >> 12) & 63]);
    out.push_back(i + 1 < size ? alphabet[(value >> 6) & 63] : '=');
    out.push_back(i + 2 < size ? alphabet[value & 63] : '=');
  }
  return out;
}

bool fromBase64(const std::string& input, std::vector<unsigned char>& out)
{
  unsigned int value = 0;
  int bits = -8;
  for (unsigned char c : input) {
    if (c == '=') break;
    const char *pos = std::find(std::begin(alphabet), std::end(alphabet) - 1, c);
    if (pos == std::end(alphabet) - 1) continue;
    value = (value << 6) | static_cast<unsigned int>(pos - alphabet);
    bits += 6;
    if (bits >= 0) {
      out.push_back(static_cast<unsigned char>((value >> bits) & 255));
      bits -= 8;
    }
  }
  return !out.empty();
}
}

bool AIProfileManager::secureStorageAvailable()
{
#ifdef _WIN32
  return true;
#else
  return false;
#endif
}

std::string AIProfileManager::protectApiKey(const std::string& key)
{
  if (key.empty()) return {};
#ifdef _WIN32
  DATA_BLOB input{};
  input.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(key.data()));
  input.cbData = static_cast<DWORD>(key.size());
  DATA_BLOB output{};
  if (!CryptProtectData(&input, L"OpenSCAD AI API key", nullptr, nullptr, nullptr,
                        CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
  std::string result = "dpapi:" + toBase64(output.pbData, output.cbData);
  SecureZeroMemory(output.pbData, output.cbData);
  LocalFree(output.pbData);
  return result;
#else
  return {};
#endif
}

std::string AIProfileManager::revealApiKey(const std::string& stored)
{
  if (stored.rfind("dpapi:", 0) == 0) {
#ifdef _WIN32
    std::vector<unsigned char> encrypted;
    if (!fromBase64(stored.substr(6), encrypted)) return {};
    DATA_BLOB input{};
    input.pbData = encrypted.data();
    input.cbData = static_cast<DWORD>(encrypted.size());
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) return {};
    std::string result(reinterpret_cast<char *>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return result;
#else
    return {};
#endif
  }
  // Migration path for profile files written by earlier builds.
  return stored;
}

std::string AIProfileManager::apiKeyFromProfile(const nlohmann::json& profile)
{
  if (!profile.is_object()) return {};
  if (profile.contains("apiKeyEncrypted") && profile["apiKeyEncrypted"].is_string())
    return revealApiKey(profile["apiKeyEncrypted"].get<std::string>());
  return revealApiKey(profile.value("apiKey", std::string{}));
}
