/*
 * The MIT License
 *
 * Copyright (c) 2016-2026, Torsten Paul <torsten.paul@gmx.de>,
 *                          Marius Kintel <marius@kintel.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "libsvg/data.h"
#include "libsvg/libsvg.h"
#include "libsvg/shape.h"

#include <algorithm>
#include <catch2/catch_all.hpp>

#include <functional>
#include <string>
#include <vector>

namespace {

/* Marker string that must never end up in imported shape data. */
constexpr const char *kCanary = "OPENSCAD-XXE-CANARY-DO-NOT-LEAK";

std::string svg_path(const std::string& name)
{
  return std::string(OPENSCAD_TEST_DATA_DIR) + "/svg/" + name;
}

/* Text content of every "data" shape (one is created for each XML text node)
 * in the given shape tree. */
std::vector<std::string> collect_text(const libsvg::shapes_list_t& shapes)
{
  std::vector<std::string> texts;
  std::function<void(std::vector<std::string>&, const libsvg::shape *)> append =
    [&](std::vector<std::string>& out, const libsvg::shape *s) {
      if (s == nullptr) {
        return;
      }
      if (const auto *d = dynamic_cast<const libsvg::data *>(s)) {
        out.push_back(d->get_text());
      }
      for (const auto *child : s->get_children()) {
        append(out, child);
      }
    };
  for (const auto& shape_ptr : shapes) {
    append(texts, shape_ptr.get());
  }
  return texts;
}

std::string all_text(libsvg::shapes_list_t *shapes)
{
  std::string result;
  for (const auto& text : collect_text(*shapes)) {
    result += text;
  }
  return result;
}

bool has_shape_named(libsvg::shapes_list_t *shapes, const std::string& name)
{
  for (const auto& shape_ptr : *shapes) {
    if (shape_ptr->get_name() == name) {
      return true;
    }
  }
  return false;
}

libsvg::shapes_list_t *read(const std::string& svg)
{
  fnContext ctx(nullptr, 0);
  return libsvg::libsvg_read_file(svg.c_str(), &ctx);
}

}  // namespace

/* ------------------------------------------------------------------ */

TEST_CASE("SVG import does not expand external XML entities (XXE)", "[libsvg][Security]")
{
  SECTION("the entity is not substituted into any shape text")
  {
    auto shapes = read(svg_path("xxe.svg"));
    const std::string text = all_text(shapes);
    CHECK(text.find(kCanary) == std::string::npos);
    libsvg::libsvg_free(shapes);
  }

  SECTION("the rest of the SVG is still imported")
  {
    auto shapes = read(svg_path("xxe.svg"));
    CHECK(has_shape_named(shapes, "rect"));
    CHECK(all_text(shapes).find(kCanary) == std::string::npos);
    libsvg::libsvg_free(shapes);
  }
}

TEST_CASE("SVG import is not vulnerable to entity expansion bombs", "[libsvg][Security]")
{
  SECTION("parsing succeeds and does not expand the entities")
  {
    auto shapes = read(svg_path("bomb.svg"));
    const std::string text = all_text(shapes);
    CHECK(text.size() < 64);
    libsvg::libsvg_free(shapes);
  }
}

TEST_CASE("SVG import does not load external DTDs", "[libsvg][Security]")
{
  SECTION("neither the DTD load nor the entity substitution happens")
  {
    auto shapes = read(svg_path("external-dtd.svg"));
    const std::string text = all_text(shapes);
    CHECK(text.find(kCanary) == std::string::npos);
    CHECK(has_shape_named(shapes, "rect"));
    libsvg::libsvg_free(shapes);
  }
}

TEST_CASE("benign SVG text is still imported", "[libsvg][Security]")
{
  SECTION("plain text nodes are imported as before")
  {
    auto shapes = read(svg_path("benign.svg"));
    const auto texts = collect_text(*shapes);
    CHECK(std::find(texts.begin(), texts.end(), "hello") != texts.end());
    libsvg::libsvg_free(shapes);
  }
}
