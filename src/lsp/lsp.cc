#include "lsp/lsp.h"


#include <algorithm>
#include <iostream>
#include <stdio.h>

DocumentUri DocumentUri::fromPath(const std::string &path) {
  DocumentUri result;
  result.setPath(path);
  return result;
}

void DocumentUri::setPath(const std::string &path) {
  // file:///c%3A/Users/jacob/Desktop/superindex/indexer/full_tests
  raw_uri = path;

  size_t index = raw_uri.find(':');
  if (index == 1) { // widows drive letters must always be 1 char
    raw_uri.replace(raw_uri.begin() + index, raw_uri.begin() + index + 1,
                    "%3A");
  }

  // subset of reserved characters from the URI standard
  // http://www.ecma-international.org/ecma-262/6.0/#sec-uri-syntax-and-semantics
  std::string t;
  t.reserve(8 + raw_uri.size());
  // TODO: proper fix
#if defined(_WIN32)
  t += "file:///";
#else
  t += "file://";
#endif

  // clang-format off
  for (char c : raw_uri)
    switch (c) {
    case ' ': t += "%20"; break;
    case '#': t += "%23"; break;
    case '$': t += "%24"; break;
    case '&': t += "%26"; break;
    case '(': t += "%28"; break;
    case ')': t += "%29"; break;
    case '+': t += "%2B"; break;
    case ',': t += "%2C"; break;
    case ';': t += "%3B"; break;
    case '?': t += "%3F"; break;
    case '@': t += "%40"; break;
    default: t += c; break;
    }
  // clang-format on
  raw_uri = std::move(t);
}

std::string DocumentUri::getPath() const {
  if (raw_uri.compare(0, 7, "file://")) {
    std::cerr << "Received potentially bad URI (not starting with file://): " << raw_uri;
    return raw_uri;
  }
  std::string ret;
#ifdef _WIN32
  // Skipping the initial "/" on Windows
  size_t i = 8;
#else
  size_t i = 7;
#endif
  auto from_hex = [](unsigned char c) {
    return c - '0' < 10 ? c - '0' : (c | 32) - 'a' + 10;
  };
  for (; i < raw_uri.size(); i++) {
    if (i + 3 <= raw_uri.size() && raw_uri[i] == '%') {
      ret.push_back(from_hex(raw_uri[i + 1]) * 16 + from_hex(raw_uri[i + 2]));
      i += 2;
    } else
      ret.push_back(raw_uri[i]);
  }
#ifdef _WIN32
  std::replace(ret.begin(), ret.end(), '\\', '/');
  if (ret.size() > 1 && ret[0] >= 'a' && ret[0] <= 'z' && ret[1] == ':') {
    ret[0] = toupper(ret[0]);
  }
#endif
  return ret;
}

std::string Position::toString() const {
  return std::to_string(line) + ":" + std::to_string(character);
}
