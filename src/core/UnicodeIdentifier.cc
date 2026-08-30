#include "core/UnicodeIdentifier.h"

#include <glib.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

#include "core/UnicodeIdentifierTables.h"

namespace {

using UnicodeIdentifierTables::Range;

bool contains(const Range *ranges, size_t count, uint32_t codepoint)
{
  const auto *above =
    std::upper_bound(ranges, ranges + count, codepoint,
                     [](uint32_t value, const Range& range) { return value < range.first; });
  return above != ranges && codepoint <= (above - 1)->last;
}

// ASCII is deliberately left out of the generated tables, as the lexer already
// selects it with its own character classes. The two predicates below must stay
// in sync with IDSTART and IDREST in lexer.l: normalisation can turn a non-ASCII
// code point into an ASCII one (U+212A KELVIN SIGN becomes U+004B, U+037E GREEK
// QUESTION MARK becomes U+003B), so the check cannot simply pass ASCII through.
bool isAsciiStart(uint32_t codepoint)
{
  return (codepoint >= 'a' && codepoint <= 'z') || (codepoint >= 'A' && codepoint <= 'Z') ||
         codepoint == '_' || codepoint == '$';
}

bool isAsciiContinue(uint32_t codepoint)
{
  return (codepoint >= 'a' && codepoint <= 'z') || (codepoint >= 'A' && codepoint <= 'Z') ||
         (codepoint >= '0' && codepoint <= '9') || codepoint == '_';
}

bool isStart(uint32_t codepoint)
{
  if (codepoint < 0x80) return isAsciiStart(codepoint);
  return contains(UnicodeIdentifierTables::ID_START, std::size(UnicodeIdentifierTables::ID_START),
                  codepoint);
}

bool isContinue(uint32_t codepoint)
{
  if (codepoint < 0x80) return isAsciiContinue(codepoint);
  return contains(UnicodeIdentifierTables::ID_CONTINUE, std::size(UnicodeIdentifierTables::ID_CONTINUE),
                  codepoint);
}

}  // namespace

namespace UnicodeIdentifier {

Result normalize(const char *text, size_t length)
{
  Result result;

  // g_utf8_normalize() requires well-formed input. The lexer pattern only
  // approximates UTF-8: it accepts overlong forms, surrogates and code points
  // above U+10FFFF, so the encoding has to be checked here.
  if (!g_utf8_validate(text, static_cast<gssize>(length), nullptr)) {
    result.status = Status::InvalidEncoding;
    return result;
  }

  gchar *normalized = g_utf8_normalize(text, static_cast<gssize>(length), G_NORMALIZE_NFC);
  if (normalized == nullptr) {
    result.status = Status::InvalidEncoding;
    return result;
  }

  for (const gchar *pos = normalized; *pos != '\0'; pos = g_utf8_next_char(pos)) {
    const gunichar codepoint = g_utf8_get_char(pos);
    const bool first = (pos == normalized);
    if (first ? isStart(codepoint) : isContinue(codepoint)) continue;

    result.status = first ? Status::InvalidStart : Status::InvalidContinue;
    result.codepoint = codepoint;
    g_free(normalized);
    return result;
  }

  result.identifier = normalized;
  g_free(normalized);
  return result;
}

std::string formatCodePoint(uint32_t codepoint)
{
  std::ostringstream stream;
  stream << "U+" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << codepoint;
  return stream.str();
}

const char *unicodeVersion()
{
  return UnicodeIdentifierTables::UNICODE_VERSION;
}

}  // namespace UnicodeIdentifier
