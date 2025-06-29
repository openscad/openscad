/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2025 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#include <fmt/args.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "core/Value.h"
#include "utils/printutils.h"

template <>
struct fmt::formatter<RangeType> : public fmt::formatter<double> {
  template <typename FmtContext>
  constexpr auto format(RangeType const& r, FmtContext& ctx) const {
    format_to(ctx.out(), "[");
    fmt::formatter<double>::format(r.begin_value(), ctx);
    if (r.step_value() != 1) {
      format_to(ctx.out(), ":");
      fmt::formatter<double>::format(r.step_value(), ctx);
    }
    format_to(ctx.out(), ":");
    fmt::formatter<double>::format(r.end_value(), ctx);
    format_to(ctx.out(), "]");
    return ctx.out();
  }
};

template <>
class fmt::formatter<VectorType> : public fmt::formatter<std::string> {
public:
  template <typename FmtContext>
  auto format(VectorType const& v, FmtContext& ctx) const {
    if (!v.empty()) {
      fmt::format_to(ctx.out(), "[");
      auto it = v.begin();
      while (true) {
        fmt::format_to(ctx.out(), "{}", *it);
        if (++it == v.end()) {
            break;
        }
        fmt::format_to(ctx.out(), ", ");
      }
      fmt::format_to(ctx.out(), "]");
    }
    return ctx.out();
  }
};

template <>
class fmt::formatter<Value> {
private:
  std::string spec;
public:
  auto parse(format_parse_context& ctx) {
      spec = "";
      auto i = ctx.begin(), end = ctx.end();
      while (i != end && *i != '}') {
        char c = *i++;
        spec.push_back(c);
      };
      spec = spec.empty() ? "{}" : "{:" + spec + "}";
      return i;
  }

  template <typename FmtContext>
  constexpr auto format(Value const& v, FmtContext& ctx) const {
      switch (v.type()) {
        case Value::Type::UNDEFINED:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toUndefString());
        case Value::Type::BOOL:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toBool());
        case Value::Type::NUMBER:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toDouble());
        case Value::Type::STRING:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toString());
        case Value::Type::RANGE:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toRange());
        case Value::Type::VECTOR:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), v.toVector());
        case Value::Type::FUNCTION:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), STR(v.toFunction()));
        case Value::Type::OBJECT:
          return fmt::format_to(ctx.out(), fmt::format_string<>(spec), STR(v.toObject()));
        default:
          return ctx.out();
      }
  }
};
