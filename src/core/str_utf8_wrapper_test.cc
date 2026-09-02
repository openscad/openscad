#include "core/str_utf8_wrapper.h"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("normalize_utf8_nfc composes canonically equivalent spellings", "[core][str_utf8_wrapper]")
{
  SECTION("Decomposed input is composed")
  {
    CHECK(normalize_utf8_nfc("ho\u0308he") == "h\u00f6he");
    CHECK(normalize_utf8_nfc("ma\u030ass") == "m\u00e5ss");
  }

  SECTION("Already composed input is returned unchanged")
  {
    CHECK(normalize_utf8_nfc("h\u00f6he") == "h\u00f6he");
  }

  SECTION("Canonical singletons are folded")
  {
    CHECK(normalize_utf8_nfc("\u2126") == "\u03a9");
    CHECK(normalize_utf8_nfc("\u212a") == "K");
  }

  SECTION("Compatibility equivalents are left alone")
  {
    // NFC, not NFKC: the micro sign stays distinct from the Greek letter, and
    // superscripts keep their form.
    CHECK(normalize_utf8_nfc("\u00b5") == "\u00b5");
    CHECK(normalize_utf8_nfc("mm\u00b2") == "mm\u00b2");
  }
}

TEST_CASE("normalize_utf8_nfc passes through what it cannot normalise", "[core][str_utf8_wrapper]")
{
  SECTION("ASCII takes the fast path")
  {
    CHECK(normalize_utf8_nfc("") == "");
    CHECK(normalize_utf8_nfc("firstSet") == "firstSet");
    CHECK(normalize_utf8_nfc("Name.dot") == "Name.dot");
  }

  SECTION("Malformed UTF-8 is returned unchanged")
  {
    const std::string malformed("a\xed\xa0\x80\x62");
    CHECK(normalize_utf8_nfc(malformed) == malformed);
  }

  SECTION("Strings containing a null byte are passed through untouched")
  {
    // Normalising these would truncate at the null byte, which could turn two
    // distinct names into the same lookup key.
    const std::string withNull("ho\u0308he\0he", 9);
    REQUIRE(withNull.size() == 9);
    CHECK(normalize_utf8_nfc(withNull) == withNull);
  }
}
