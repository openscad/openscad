// Inspired by ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <vector>
#include <string>


// 201402L is C++14
#if __cplusplus > 201402L
    #include <optional>
    template<typename _T>
    using OptionalType = std::optional<_T>;
#else
    // Fallback to boost optional for older C++ standards
    #include <boost/optional.hpp>
    template<typename _T>
    using OptionalType = boost::optional<_T>;
#endif


struct RequestId {
    enum { UNSET, STRING, INT, AUTO_INCREMENT } type = UNSET;
    std::string value_str;
    int value_int = 0;

    bool is_set() const { return type != UNSET; }
    std::string value() const {
        switch(type) {
        case STRING:
            return value_str;
        case INT:
            return std::to_string(value_int);
        case AUTO_INCREMENT:
            return "<AUTOINCREMENT-UNSET>";
        case UNSET:
            return "<UNSET>";
        }
        return "";
    }
};

enum class ErrorCode {
  // Defined by JSON RPC
  ParseError = -32700,
  InvalidRequest = -32600,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,
  serverErrorStart = -32099,
  serverErrorEnd = -32000,
  ServerNotInitialized = -32002,
  UnknownErrorCode = -32001,

  // Defined by the protocol.
  RequestCancelled = -32800,
};

struct DocumentUri {
  static DocumentUri fromPath(const std::string &path);

  bool operator==(const DocumentUri &o) const { return raw_uri == o.raw_uri; }
  bool operator<(const DocumentUri &o) const { return raw_uri < o.raw_uri; }

  void setPath(const std::string &path);
  std::string getPath() const;

  std::string raw_uri;
};

struct Position {
  int line = 0;
  int character = 0;
  bool operator==(const Position &o) const {
    return line == o.line && character == o.character;
  }
  bool operator<(const Position &o) const {
    return line != o.line ? line < o.line : character < o.character;
  }
  bool operator<=(const Position &o) const {
    return line != o.line ? line < o.line : character <= o.character;
  }
  std::string toString() const;
};

struct lsRange {
  Position start;
  Position end;
  bool operator==(const lsRange &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const lsRange &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
  bool includes(const lsRange &o) const {
    return start <= o.start && o.end <= end;
  }
  bool intersects(const lsRange &o) const {
    return start < o.end && o.start < end;
  }
};

struct lsLocation {
  DocumentUri uri;
  lsRange range;
  bool operator==(const lsLocation &o) const {
    return uri == o.uri && range == o.range;
  }
  bool operator<(const lsLocation &o) const {
    return !(uri == o.uri) ? uri < o.uri : range < o.range;
  }
};

struct LocationLink {
  std::string targetUri;
  lsRange targetRange;
  lsRange targetSelectionRange;
  explicit operator bool() const { return targetUri.size(); }
  explicit operator lsLocation() && {
    return {DocumentUri{std::move(targetUri)}, targetSelectionRange};
  }
  bool operator==(const LocationLink &o) const {
    return targetUri == o.targetUri &&
           targetSelectionRange == o.targetSelectionRange;
  }
  bool operator<(const LocationLink &o) const {
    return !(targetUri == o.targetUri)
               ? targetUri < o.targetUri
               : targetSelectionRange < o.targetSelectionRange;
  }
};

enum class SymbolKind : uint8_t {
  Unknown = 0,

  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
};

struct SymbolInformation {
  std::string name;
  SymbolKind kind;
  lsLocation location;
  OptionalType<std::string> containerName;
};

struct TextDocumentIdentifier {
  DocumentUri uri;
};

struct VersionedTextDocumentIdentifier {
  DocumentUri uri;
  // The version number of this document.  number | null
  OptionalType<int> version;
};

struct TextEdit {
  lsRange range;
  std::string newText;
};

struct TextDocumentItem {
  DocumentUri uri;
  std::string languageId;
  int version;
  std::string text;
};

struct TextDocumentContentChangeEvent {
  // The range of the document that changed.
  OptionalType<lsRange> range;
  // The length of the range that got replaced.
  OptionalType<int> rangeLength;
  // The new text of the range/document.
  std::string text;
};

struct TextDocumentDidChangeParam {
  VersionedTextDocumentIdentifier textDocument;
  std::vector<TextDocumentContentChangeEvent> contentChanges;
};

struct WorkDoneProgress {
  std::string kind;
  OptionalType<std::string> title;
  OptionalType<std::string> message;
  OptionalType<int> percentage;
};
struct WorkDoneProgressParam {
  std::string token;
  WorkDoneProgress value;
};

struct WorkspaceFolder {
  DocumentUri uri;
  std::string name;
};

enum class MessageType {
    Error = 1,
    Warning = 2,
    Info = 3,
    Log = 4,
};
