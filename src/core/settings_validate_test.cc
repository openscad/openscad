#include <catch2/catch_all.hpp>

#include "core/Settings.h"

using Settings::SettingsExport3mf;
using Settings::SettingsExportPdf;
using Settings::SettingsExportSvg;

TEST_CASE("Settings entries validate encoded values", "[Settings]")
{
  SECTION("bool")
  {
    const auto& entry = SettingsExportSvg::exportSvgFill;
    CHECK(entry.validate("true"));
    CHECK(entry.validate("false"));
    CHECK(entry.validate(" true "));
    CHECK(entry.validate("1"));
    CHECK(entry.validate("0"));
    CHECK_FALSE(entry.validate("maybe"));
    CHECK_FALSE(entry.validate(""));
  }

  SECTION("int, including the range")
  {
    const auto& entry = SettingsExport3mf::export3mfDecimalPrecision;  // 1 to 16
    CHECK(entry.validate("1"));
    CHECK(entry.validate("16"));
    CHECK_FALSE(entry.validate("0"));
    CHECK_FALSE(entry.validate("17"));
    CHECK_FALSE(entry.validate("6.5"));
    CHECK_FALSE(entry.validate("abc"));
  }

  SECTION("double, including the range")
  {
    const auto& entry = SettingsExportSvg::exportSvgStrokeWidth;  // 0 to 999
    CHECK(entry.validate("0"));
    CHECK(entry.validate("0.5"));
    CHECK(entry.validate("999"));
    CHECK_FALSE(entry.validate("-1"));
    CHECK_FALSE(entry.validate("1000"));
    CHECK_FALSE(entry.validate("nan"));
    CHECK_FALSE(entry.validate("abc"));
  }

  SECTION("enum")
  {
    const auto& entry = SettingsExportPdf::exportPdfPaperSize;
    CHECK(entry.validate("a4"));
    CHECK(entry.validate("letter"));
    CHECK_FALSE(entry.validate("A4"));
    CHECK_FALSE(entry.validate("b5"));
  }

  SECTION("string accepts anything")
  {
    const auto& entry = SettingsExportSvg::exportSvgFillColor;
    CHECK(entry.validate("red"));
    CHECK(entry.validate(""));
  }
}
