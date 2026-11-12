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

/* Number of points in the path data of all shapes. */
size_t path_point_count(const libsvg::shapes_list_t& shapes)
{
  size_t count = 0;
  for (const auto& shape_ptr : shapes) {
    for (const auto& path : shape_ptr->get_path_list()) {
      count += path.size();
    }
  }
  return count;
}

libsvg::shapes_list_t *read(const std::string& svg)
{
  fnContext ctx(nullptr, 0);
  return libsvg::libsvg_read_file(svg.c_str(), &ctx);
}

/* Like read(), but returns nullptr if the file is rejected as invalid. */
libsvg::shapes_list_t *read_or_null(const std::string& svg)
{
  try {
    return read(svg);
  } catch (const libsvg::SvgException&) {
    return nullptr;
  }
}

/* Temporarily lifts the <use> amplification cap, restoring the previous
 * value on scope exit even if a check fails in between. */
class unlimited_use_guard
{
public:
  unlimited_use_guard() : previous(libsvg::unlimited_use_cloned_shapes)
  {
    libsvg::unlimited_use_cloned_shapes = true;
  }

  ~unlimited_use_guard() { libsvg::unlimited_use_cloned_shapes = previous; }

private:
  bool previous;
};

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

TEST_CASE("SVG import does not load files through external parameter entities", "[libsvg][Security]")
{
  SECTION("the file content does not end up in path data")
  {
    /* Depending on the libxml2 version the undefined entity either makes the
     * import fail or leaves the path empty; either way no path data may come
     * from xxe-path-data.txt. */
    auto shapes = read_or_null(svg_path("xxe-parameter-entity.svg"));
    if (shapes != nullptr) {
      CHECK(path_point_count(*shapes) == 0);
      libsvg::libsvg_free(shapes);
    } else {
      SUCCEED("the file was rejected");
    }
  }
}

TEST_CASE("SVG import is not vulnerable to entity expansion bombs", "[libsvg][Security]")
{
  SECTION("the entities are not expanded")
  {
    /* libxml2 2.9.x rejects this many nested entity levels on its own, so the
     * import may fail; if it succeeds, nothing may have been expanded. */
    auto shapes = read_or_null(svg_path("bomb.svg"));
    if (shapes != nullptr) {
      CHECK(all_text(shapes).find("lol") == std::string::npos);
      libsvg::libsvg_free(shapes);
    } else {
      SUCCEED("the file was rejected");
    }
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

TEST_CASE("internal entities in attributes are still expanded", "[libsvg][Security]")
{
  SECTION("path data given through an internal entity is imported")
  {
    auto shapes = read(svg_path("internal-entity-attribute.svg"));
    CHECK(has_shape_named(shapes, "path"));
    CHECK(path_point_count(*shapes) > 0);
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

TEST_CASE("SVG import caps shape amplification from <use> elements", "[libsvg][Security]")
{
  SECTION("a <use> bomb is rejected instead of materializing unbounded shapes")
  {
    /* 600 <use> tags reference a 200-shape def, which would clone
     * 600 x 201 = 120,600 shapes into the global shape list. The
     * amplification cap aborts the import; read_or_null() maps the
     * SvgException to nullptr. */
    auto shapes = read_or_null(svg_path("use-bomb.svg"));
    CHECK(shapes == nullptr);
  }

  SECTION("benign <use> repetition below the cap is still imported in full")
  {
    /* 3 uses of a 2-shape def must yield exactly 13 shapes: the root <svg>
     * element plus 3 x (1 use element + 1 cloned group + 2 cloned rects).
     * No truncation and no rejection, i.e. the cap does not affect
     * legitimate SVG. */
    auto shapes = read(svg_path("use-repetition.svg"));
    REQUIRE(shapes != nullptr);
    CHECK(shapes->size() == 13);
    libsvg::libsvg_free(shapes);
  }

  SECTION("the next import succeeds after a rejected bomb")
  {
    /* The rejection must not leave stale process-global state behind:
     * libsvg_read_file() resets its counter even when it throws. */
    read_or_null(svg_path("use-bomb.svg"));
    auto shapes = read(svg_path("benign.svg"));
    REQUIRE(shapes != nullptr);
    CHECK(has_shape_named(shapes, "rect"));
    libsvg::libsvg_free(shapes);
  }
}

TEST_CASE("the SVG <use> amplification cap can be removed for advanced users", "[libsvg][Security]")
{
  SECTION("with the cap removed the bomb is imported in full")
  {
    unlimited_use_guard guard;
    auto shapes = read(svg_path("use-bomb.svg"));
    REQUIRE(shapes != nullptr);
    /*
     * 1 root <svg> element
     * + 600 <use> elements
     * + 600 x (1 cloned group + 200 cloned rects)
     * = 121,201 shapes, i.e. no truncation.
     */
    CHECK(shapes->size() == 121'201);
    libsvg::libsvg_free(shapes);
  }
}
