#include "core/UnicodeIdentifier.h"

#include <catch2/catch_all.hpp>
#include <string>

using UnicodeIdentifier::Status;

static UnicodeIdentifier::Result check(const std::string& token)
{
  return UnicodeIdentifier::normalize(token.data(), token.size());
}

TEST_CASE("UnicodeIdentifier accepts letters outside ASCII", "[core][UnicodeIdentifier]")
{
  SECTION("Latin letters with diacritics and sharp s")
  {
    CHECK(check("gr\u00f6\u00dfe").identifier == "gr\u00f6\u00dfe");
    CHECK(check("Fu\u1e9edicke").identifier == "Fu\u1e9edicke");
    CHECK(check("na\u00efve").status == Status::Valid);
  }

  SECTION("Greek letters commonly used for maths")
  {
    CHECK(check("\u03c0").identifier == "\u03c0");
    CHECK(check("\u0394x").identifier == "\u0394x");
    CHECK(check("\u03bcm").identifier == "\u03bcm");
  }

  SECTION("Other scripts")
  {
    CHECK(check("\u4e2d\u6587").status == Status::Valid);
    CHECK(check("\u0440\u0430\u0437\u043c\u0435\u0440").status == Status::Valid);
  }

  SECTION("Special variables keep their dollar sign")
  {
    CHECK(check("$gr\u00f6\u00dfe").identifier == "$gr\u00f6\u00dfe");
  }
}

TEST_CASE("UnicodeIdentifier normalises to NFC", "[core][UnicodeIdentifier]")
{
  SECTION("Decomposed and precomposed spellings name the same identifier")
  {
    const auto decomposed = check("gro\u0308\u00dfe");
    const auto precomposed = check("gr\u00f6\u00dfe");
    REQUIRE(decomposed.status == Status::Valid);
    REQUIRE(precomposed.status == Status::Valid);
    CHECK(decomposed.identifier == precomposed.identifier);
  }

  SECTION("Canonical singletons fold onto their preferred spelling")
  {
    // U+2126 OHM SIGN, U+212A KELVIN SIGN and U+212B ANGSTROM SIGN are excluded
    // from the UTS #39 profile because NFC turns them into these characters.
    CHECK(check("\u2126").identifier == "\u03a9");
    CHECK(check("\u212a").identifier == "K");
    CHECK(check("\u212b").identifier == "\u00c5");
  }
}

TEST_CASE("UnicodeIdentifier rejects code points outside the profile", "[core][UnicodeIdentifier]")
{
  SECTION("Invisible characters")
  {
    // Variation selectors are XID_Continue, the UTS #39 profile removes them.
    const auto selector = check("a\ufe0fb");
    CHECK(selector.status == Status::InvalidContinue);
    CHECK(selector.codepoint == 0xFE0F);

    CHECK(check("a\u200cb").status == Status::InvalidContinue);
    CHECK(check("a\u200db").status == Status::InvalidContinue);
  }

  SECTION("Characters that look like identifier characters")
  {
    // U+00B5 MICRO SIGN is confusable with U+03BC GREEK SMALL LETTER MU and NFC
    // does not fold the two. See issue #737.
    const auto micro = check("\u00b5m");
    CHECK(micro.status == Status::InvalidStart);
    CHECK(micro.codepoint == 0x00B5);
  }

  SECTION("Symbols, punctuation and emoji")
  {
    CHECK(check("\u2211").status == Status::InvalidStart);
    CHECK(check("\u2603").status == Status::InvalidStart);
    CHECK(check("\U0001f600").status == Status::InvalidStart);
    CHECK(check("a\u00b0").status == Status::InvalidContinue);
  }

  SECTION("Normalisation must not smuggle in ASCII punctuation")
  {
    // U+037E GREEK QUESTION MARK normalises to U+003B SEMICOLON.
    const auto question = check("a\u037eb");
    CHECK(question.status == Status::InvalidContinue);
    CHECK(question.codepoint == 0x003B);
  }

  SECTION("Malformed UTF-8")
  {
    CHECK(check(std::string("a\xed\xa0\x80\x62")).status == Status::InvalidEncoding);
    CHECK(check(std::string("a\xc3\x28")).status == Status::InvalidEncoding);
    CHECK(check(std::string("a\xf5\x80\x80\x80")).status == Status::InvalidEncoding);
  }
}

TEST_CASE("UnicodeIdentifier formats code points for diagnostics", "[core][UnicodeIdentifier]")
{
  CHECK(UnicodeIdentifier::formatCodePoint(0x00DF) == "U+00DF");
  CHECK(UnicodeIdentifier::formatCodePoint(0x1F600) == "U+1F600");
  CHECK(std::string(UnicodeIdentifier::unicodeVersion()).empty() == false);
}
