#include <catch2/catch_all.hpp>
#include <memory>
#include <sstream>
#include <string>

#include "core/SourceFile.h"
#include "core/customizer/CommentParser.h"
#include "io/export.h"
#include "openscad.h"

namespace {

std::string exportParameters(const std::string& text)
{
  SourceFile *file = nullptr;
  const bool ok = parse(file, text, "test.scad", "test.scad", false);
  const std::unique_ptr<SourceFile> holder(file);
  REQUIRE(ok);
  CommentParser::collectParameters(text, file);
  std::ostringstream out;
  REQUIRE(export_param(file, "test.scad", out));
  return out.str();
}

}  // namespace

TEST_CASE("Customizer descriptions and annotations work with CRLF line endings", "[customizer]")
{
  const std::string lf =
    "/* [Parameters] */\n"
    "// The width\n"
    "width = 10; // [0:100]\n"
    "// The shape\n"
    "shape = \"round\"; // [round, square]\n"
    "/* [Hidden] */\n"
    "foo = 1;\n"
    "cube(width);\n";
  std::string crlf;
  for (const char c : lf) {
    if (c == '\n') crlf += '\r';
    crlf += c;
  }

  const std::string expected = exportParameters(lf);
  // The annotations were parsed: a range and the options.
  CHECK(expected.find("\"max\":100.0") != std::string::npos);
  CHECK(expected.find("\"options\"") != std::string::npos);
  CHECK(exportParameters(crlf) == expected);
}
