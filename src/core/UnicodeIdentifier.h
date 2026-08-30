#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * Validation and normalisation of non-ASCII identifiers.
 *
 * The accepted character set is the default identifier syntax of Unicode
 * Standard Annex #31, restricted by the general security profile of Unicode
 * Technical Standard #39:
 *
 *   ID_Start    := (XID_Start + { $, _ }) & Identifier_Status=Allowed
 *   ID_Continue := XID_Continue & Identifier_Status=Allowed
 *
 * XID_Start and XID_Continue are the same sets C++23 and Rust use. The
 * intersection with the UTS #39 profile additionally removes code points that
 * are invisible (variation selectors are XID_Continue but not Allowed),
 * obsolete, or canonical duplicates of other identifier characters.
 *
 * Identifiers are normalised to NFC before they are validated, so that
 * canonically equivalent spellings name the same variable. Normalising first
 * also accepts the canonical singletons - U+2126 OHM SIGN, U+212A KELVIN SIGN
 * and U+212B ANGSTROM SIGN - which are excluded from the UTS #39 profile
 * precisely because NFC folds them into U+03A9, U+004B and U+00C5.
 *
 * The tables in UnicodeIdentifierTables.h are closed under NFC, so normalising
 * an accepted identifier can never turn it into a rejected one.
 */
namespace UnicodeIdentifier {

/** Why a token was not accepted as an identifier. */
enum class Status {
  Valid,
  InvalidEncoding,  //!< not well-formed UTF-8
  InvalidStart,     //!< first code point may not start an identifier
  InvalidContinue,  //!< code point may not appear inside an identifier
};

struct Result {
  Status status{Status::Valid};
  /** The NFC normalised identifier. Only set when status is Status::Valid. */
  std::string identifier;
  /** The offending code point. Only set when status is not Status::Valid. */
  uint32_t codepoint{0};
};

/**
 * Normalise the given token to NFC and check it against the identifier profile.
 * The token is expected to be UTF-8 and to contain at least one non-ASCII byte;
 * pure ASCII identifiers are handled by the lexer without calling this.
 */
Result normalize(const char *text, size_t length);

/** Format a code point as "U+00DF", for use in diagnostics. */
std::string formatCodePoint(uint32_t codepoint);

/** Version of the Unicode Character Database the tables were generated from. */
const char *unicodeVersion();

}  // namespace UnicodeIdentifier
