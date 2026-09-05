/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
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

#include <algorithm>
#include <boost/format.hpp>
#include <boost/math/interpolators/cardinal_cubic_b_spline.hpp>
#include <boost/math/interpolators/makima.hpp>
#include <boost/math/interpolators/catmull_rom.hpp>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/AST.h"
#include "core/Arguments.h"
#include "core/Builtins.h"
#include "core/Context.h"
#include "core/EvaluationSession.h"
#include "core/Expression.h"
#include "core/FreetypeRenderer.h"
#include "core/Parameters.h"
#include "core/UserModule.h"
#include "core/function.h"
#include "io/fileutils.h"
#include "io/import.h"
#include "utils/boost-utils.h"
#include "utils/degree_trig.h"
#include "utils/printutils.h"
#include "version.h"
// hash double
#include "geometry/linalg.h"

#if defined __WIN32__ || defined _MSC_VER
#include <process.h>
int process_id = _getpid();
#else
#include <sys/types.h>
#include <unistd.h>
int process_id = getpid();
#endif

std::mt19937 deterministic_rng(std::time(nullptr) + process_id);
void initialize_rng()
{
  static uint64_t seed_val = 0;
  seed_val ^= uint64_t(std::time(nullptr) + process_id);
  deterministic_rng.seed(seed_val);
  std::uniform_int_distribution<uint64_t> distributor(0);
  seed_val ^= distributor(deterministic_rng);
}

static inline bool check_arguments(const char *function_name, const Arguments& arguments,
                                   const Location& loc, unsigned int expected_count, bool warn = true)
{
  if (arguments.size() != expected_count) {
    if (warn) {
      print_argCnt_warning(function_name, arguments.size(), STR(expected_count), loc,
                           arguments.documentRoot());
    }
    return false;
  }
  return true;
}
/* // Commented due to compiler warning of unused function.
   static inline bool try_check_arguments(const Arguments& arguments, int expected_count)
   {
   return check_arguments(nullptr, arguments, Location::NONE, expected_count, false);
   }
 */
template <size_t N>
static inline bool check_arguments(const char *function_name, const Arguments& arguments,
                                   const Location& loc, const Value::Type (&expected_types)[N],
                                   bool warn = true)
{
  if (!check_arguments(function_name, arguments, loc, N, warn)) {
    return false;
  }
  for (size_t i = 0; i < N; i++) {
    if (arguments[i]->type() != expected_types[i]) {
      if (warn) {
        print_argConvert_positioned_warning(function_name, "argument " + STR(i), arguments[i]->clone(),
                                            {expected_types[i]}, loc, arguments.documentRoot());
      }
      return false;
    }
  }
  return true;
}

template <size_t N>
static inline bool try_check_arguments(const Arguments& arguments,
                                       const Value::Type (&expected_types)[N])
{
  return check_arguments(nullptr, arguments, Location::NONE, expected_types, false);
}

Value builtin_abs(Arguments arguments, const Location& loc)
{
  if (!check_arguments("abs", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {std::fabs(arguments[0]->toDouble())};
}

Value builtin_sign(Arguments arguments, const Location& loc)
{
  if (!check_arguments("sign", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  double x = arguments[0]->toDouble();
  return {(x < 0) ? -1.0 : ((x > 0) ? 1.0 : 0.0)};
}

Value builtin_rands(Arguments arguments, const Location& loc)
{
  if (arguments.size() < 3 || arguments.size() > 4) {
    print_argCnt_warning("rands", arguments.size(), "3 or 4", loc, arguments.documentRoot());
    return Value::undefined.clone();
  } else if (arguments.size() == 3) {
    if (!check_arguments("rands", arguments, loc,
                         {Value::Type::NUMBER, Value::Type::NUMBER, Value::Type::NUMBER})) {
      return Value::undefined.clone();
    }
  } else {
    if (!check_arguments(
          "rands", arguments, loc,
          {Value::Type::NUMBER, Value::Type::NUMBER, Value::Type::NUMBER, Value::Type::NUMBER})) {
      return Value::undefined.clone();
    }
  }

  double min = arguments[0]->toDouble();
  if (std::isinf(min) || std::isnan(min)) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "rands() range min cannot be infinite");
    min = -std::numeric_limits<double>::max() / 2;
    LOG(message_group::Warning, "resetting to %1f", min);
  }

  double max = arguments[1]->toDouble();
  if (std::isinf(max) || std::isnan(max)) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "rands() range max cannot be infinite");
    max = std::numeric_limits<double>::max() / 2;
    LOG(message_group::Warning, "resetting to %1f", max);
  }
  if (max < min) {
    double tmp = min;
    min = max;
    max = tmp;
  }

  double numresultsd = std::abs(arguments[2]->toDouble());
  if (std::isinf(numresultsd) || std::isnan(numresultsd)) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "rands() cannot create an infinite number of results");
    LOG(message_group::Warning, "resetting number of results to 1");
    numresultsd = 1;
  }
  auto numresults = boost_numeric_cast<size_t, double>(numresultsd);

  if (arguments.size() > 3) {
    auto seed = static_cast<uint32_t>(hash_floating_point(arguments[3]->toDouble()));
    deterministic_rng.seed(seed);
  }

  VectorType vec(arguments.session());
  vec.reserve(numresults);
  if (min >= max) {  // uniform_real_distribution doesn't allow min == max
    for (size_t i = 0; i < numresults; ++i) vec.emplace_back(min);
  } else {
    std::uniform_real_distribution<> distributor(min, max);
    for (size_t i = 0; i < numresults; ++i) {
      vec.emplace_back(distributor(deterministic_rng));
    }
  }
  return std::move(vec);
}

static std::vector<double> min_max_arguments(const Arguments& arguments, const Location& loc,
                                             const char *function_name)
{
  std::vector<double> output;
  // preserve special handling of the first argument
  // as a template for vector processing
  if (arguments.size() == 0) {
    print_argCnt_warning(function_name, arguments.size(), "at least 1", loc, arguments.documentRoot());
    return {};
  } else if (arguments.size() == 1 && arguments[0]->type() == Value::Type::VECTOR) {
    const auto& elements = arguments[0]->toVector();
    if (elements.size() == 0) {
      print_argCnt_warning(function_name, elements.size(), "at least 1 vector element", loc,
                           arguments.documentRoot());
      return {};
    }
    for (size_t i = 0; i < elements.size(); i++) {
      const auto& element = elements[i];
      // 4/20/14 semantic change per discussion:
      // break on any non-number
      if (element.type() != Value::Type::NUMBER) {
        print_argConvert_positioned_warning(function_name, "vector element " + STR(i), element,
                                            {Value::Type::NUMBER}, loc, arguments.documentRoot());
        return {};
      }
      output.push_back(element.toDouble());
    }
  } else {
    for (size_t i = 0; i < arguments.size(); i++) {
      const auto& argument = arguments[i];
      // 4/20/14 semantic change per discussion:
      // break on any non-number
      if (argument->type() != Value::Type::NUMBER) {
        print_argConvert_positioned_warning(function_name, "argument " + STR(i), argument->clone(),
                                            {Value::Type::NUMBER}, loc, arguments.documentRoot());
        return {};
      }
      output.push_back(argument->toDouble());
    }
  }
  return output;
}

Value builtin_min(Arguments arguments, const Location& loc)
{
  std::vector<double> values = min_max_arguments(arguments, loc, "min");
  if (values.empty()) {
    return Value::undefined.clone();
  }
  return {*std::min_element(values.begin(), values.end())};
}

Value builtin_max(Arguments arguments, const Location& loc)
{
  std::vector<double> values = min_max_arguments(arguments, loc, "max");
  if (values.empty()) {
    return Value::undefined.clone();
  }
  return {*std::max_element(values.begin(), values.end())};
}

Value builtin_sin(Arguments arguments, const Location& loc)
{
  if (!check_arguments("sin", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {sin_degrees(arguments[0]->toDouble())};
}

Value builtin_cos(Arguments arguments, const Location& loc)
{
  if (!check_arguments("cos", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {cos_degrees(arguments[0]->toDouble())};
}

Value builtin_asin(Arguments arguments, const Location& loc)
{
  if (!check_arguments("asin", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {asin_degrees(arguments[0]->toDouble())};
}

Value builtin_acos(Arguments arguments, const Location& loc)
{
  if (!check_arguments("acos", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {acos_degrees(arguments[0]->toDouble())};
}

Value builtin_tan(Arguments arguments, const Location& loc)
{
  if (!check_arguments("tan", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {tan_degrees(arguments[0]->toDouble())};
}

Value builtin_atan(Arguments arguments, const Location& loc)
{
  if (!check_arguments("atan", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {atan_degrees(arguments[0]->toDouble())};
}

Value builtin_atan2(Arguments arguments, const Location& loc)
{
  if (!check_arguments("atan2", arguments, loc, {Value::Type::NUMBER, Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {atan2_degrees(arguments[0]->toDouble(), arguments[1]->toDouble())};
}

Value builtin_pow(Arguments arguments, const Location& loc)
{
  if (!check_arguments("pow", arguments, loc, {Value::Type::NUMBER, Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {pow(arguments[0]->toDouble(), arguments[1]->toDouble())};
}

Value builtin_round(Arguments arguments, const Location& loc)
{
  if (!check_arguments("round", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {round(arguments[0]->toDouble())};
}

Value builtin_ceil(Arguments arguments, const Location& loc)
{
  if (!check_arguments("ceil", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {ceil(arguments[0]->toDouble())};
}

Value builtin_floor(Arguments arguments, const Location& loc)
{
  if (!check_arguments("floor", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {floor(arguments[0]->toDouble())};
}

Value builtin_sqrt(Arguments arguments, const Location& loc)
{
  if (!check_arguments("sqrt", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {sqrt(arguments[0]->toDouble())};
}

Value builtin_exp(Arguments arguments, const Location& loc)
{
  if (!check_arguments("exp", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {exp(arguments[0]->toDouble())};
}

Value builtin_length(Arguments arguments, const Location& loc)
{
  if (try_check_arguments(arguments, {Value::Type::VECTOR})) {
    return {double(arguments[0]->toVector().size())};
  }
  if (try_check_arguments(arguments, {Value::Type::OBJECT})) {
    return double(arguments[0]->toObject().values().size());
  }
  if (!check_arguments("len", arguments, loc, {Value::Type::STRING})) {
    return Value::undefined.clone();
  }
  // Unicode glyph count for the length -- rather than the string (num. of bytes) length.
  return {double(arguments[0]->toStrUtf8Wrapper().get_utf8_strlen())};
}

Value builtin_log(Arguments arguments, const Location& loc)
{
  double x, y;
  if (arguments.size() == 1) {
    if (!check_arguments("log", arguments, loc, {Value::Type::NUMBER})) {
      return Value::undefined.clone();
    }
    x = 10.0;
    y = arguments[0]->toDouble();
  } else {
    if (!check_arguments("log", arguments, loc, {Value::Type::NUMBER, Value::Type::NUMBER})) {
      return Value::undefined.clone();
    }
    x = arguments[0]->toDouble();
    y = arguments[1]->toDouble();
  }
  return {log(y) / log(x)};
}

Value builtin_ln(Arguments arguments, const Location& loc)
{
  if (!check_arguments("ln", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  }
  return {log(arguments[0]->toDouble())};
}

Value builtin_str(Arguments arguments, const Location& /*loc*/)
{
  std::ostringstream stream;
  for (const auto& argument : arguments) {
    stream << argument->toString();
  }
  return {stream.str()};
}

Value builtin_chr(Arguments arguments, const Location& /*loc*/)
{
  std::ostringstream stream;
  for (const auto& argument : arguments) {
    stream << argument->chrString();
  }
  return {stream.str()};
}

Value builtin_ord(Arguments arguments, const Location& loc)
{
  if (!check_arguments("ord", arguments, loc, {Value::Type::STRING})) {
    return Value::undefined.clone();
  }
  const str_utf8_wrapper& arg_str = arguments[0]->toStrUtf8Wrapper();
  if (!arg_str.utf8_validate()) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "ord() argument '%1$s' is not a valid utf8 string", arg_str.toString());
    return Value::undefined.clone();
  }

  if (arg_str.get_utf8_strlen() == 0) {
    return Value::undefined.clone();
  }

  return {(double)arg_str.get_utf8_char()};
}

Value builtin_concat(Arguments arguments, const Location& /*loc*/)
{
  VectorType result(arguments.session());
  result.reserve(arguments.size());
  for (auto& argument : arguments) {
    if (argument->type() == Value::Type::VECTOR) {
      result.emplace_back(EmbeddedVectorType(std::move(argument->toVectorNonConst())));
    } else {
      result.emplace_back(std::move(argument.value));
    }
  }
  return std::move(result);
}

#define OBJECT_HELP                                                                             \
  "In an unnamed list, entries must be [key,value] to set or [key] to delete. The key must be " \
  "<string>."

static std::string builtin_object_unnamed(ObjectType& result, const Value& value, int arg_index)
{
  std::string prior_args = "Argument " + std::to_string(arg_index) + " ";

  switch (value.type()) {
  case Value::Type::OBJECT: {
    const auto& obj = value.toObject();
    for (const auto& key : obj.keys()) {
      result.set(key, obj.get(key).clone());
    }
    return "";
  }
  case Value::Type::VECTOR: {
    const VectorType& vector = value.toVector();
    int element = 0;
    for (const auto& member : vector) {
      std::string prior_entries = "Element " + std::to_string(element++) + " ";

      if (member.type() != Value::Type::VECTOR) {
        return str(
          boost::format("object( %s[%s<%s>] ) Entry type is not a list, it is <%s>. " OBJECT_HELP) %
          prior_args.c_str() % prior_entries.c_str() % Value::typeName(member.type()) %
          Value::typeName(member.type()));
      }

      const auto& entry = member.toVector();
      switch (entry.size()) {
      case 2:
      case 1: {
        if (entry[0].type() != Value::Type::STRING) {
          const char *es = entry.size() == 1 ? "" : ",value";
          return str(
            boost::format(
              "object(%s[%s[<%s>%s]]) The key of the entry is not <string> but <%s>. " OBJECT_HELP) %
            prior_args % prior_entries % Value::typeName(entry[0].type()) % es %
            Value::typeName(entry[0].type()));
        }
        const auto& key = entry[0].toString();
        if (entry.size() == 1) {
          result.del(key);
        } else {
          result.set(key, entry[1].clone());
        }
        break;
      };

      case 0:
        return str(boost::format("object(%s[%s[]]) Entry is empty. " OBJECT_HELP) % prior_args.c_str() %
                   prior_entries.c_str());

      default:
        return str(
          boost::format(
            "object(%s[%s[...]]) Entry length is %d, must be 1 [key] or 2 [key,value]. " OBJECT_HELP) %
          prior_args % prior_entries % entry.size());
      }
    }
    return "";
  }
  default:
    return str(boost::format(
                 "object(%s<%s>) An unnamed argument must be either <object> or <list>, it is <%s>. ") %
               prior_args % Value::typeName(value.type()) % Value::typeName(value.type()));
  }
}
/**
    The builtin_object function takes either a named or unnamed argument.
    A named argument is assigned with name=value to the result object. An unnamed
    argument is either an object or a vector. An object will have its
    entries copied, potentially overwriting earlier entries. A vector
    must be of vectors. It can contain as member vectors:

        ["k"]       deletes key k
        ["k", v]    sets key k=v

    Any other values are incorrect and will return undef and be logged as warning.

    If a value is a function and takes a "this" argument, then the function
    is converted to a method by creating a new context for it with "this"
    set to the object being created. When we parse the parameters in Parameters
    we will skip the this when the context already has it.
*/
Value builtin_object(const std::shared_ptr<const Context>& context, const FunctionCall *call)
{
  ObjectType result(context->session());
  int n = 0;
  for (const auto& argument : call->arguments) {
    Value value = argument->getExpr()->evaluate(context);
    if (argument->getName().empty()) {
      const auto error = builtin_object_unnamed(result, value, n);
      if (!error.empty()) {
        LOG(message_group::Warning, call->location(), context->documentRoot(), error.c_str());
        return Value::undef(error);
      }
    } else {
      result.set(argument->getName(), std::move(value));
    }
    n++;
  }
  std::vector<Context *> contexts;
  for (Value& value : result.ptr->values) {
    if (value.type() == Value::Type::FUNCTION) {
      auto& function = value.toFunction();
      auto it = function.findAssignmentByName(Parameters::THIS_PARAMETER);
      if (!it) continue;

      auto parent = function.getContext();
      if (parent->lookup_local_variable(Parameters::THIS_CONTEXT)) {
        // function is already a method, so take the parent context
        parent = parent->getParent();
      }
      ContextHandle<Context> ctx = Context::create<Context>(parent);
      contexts.push_back(ctx.operator->());
      Value method(FunctionType(*ctx, function.getExpr(), function.getParameters()));
      value = std::move(method);
    }
  }
  Value object(result);
  for (Context *c : contexts) {
    c->set_variable(Parameters::THIS_CONTEXT, object.clone());
  }
  return object;
}

Value builtin_has_key(Arguments arguments, const Location& loc)
{
  if (!check_arguments("has_key", arguments, loc, {Value::Type::OBJECT, Value::Type::STRING})) {
    return Value::undefined.clone();
  }
  const auto& obj = arguments[0]->toObject();
  const auto& key = arguments[1]->toString();
  return obj.contains(key);
}

Value builtin_lookup(Arguments arguments, const Location& loc)
{
  if (!check_arguments("lookup", arguments, loc, {Value::Type::NUMBER, Value::Type::VECTOR})) {
    return Value::undefined.clone();
  }
  double p = arguments[0]->toDouble();
  if (!std::isfinite(p)) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "lookup(%1$s, ...) first argument is not a number", arguments[0]->toEchoStringNoThrow());
    return Value::undefined.clone();
  }

  double low_p, low_v, high_p, high_v;
  const auto& vec = arguments[1]->toVector();

  // Second must be a vector of vec2, with valid numbers inside
  auto it = vec.begin();
  if (vec.empty() || it->toVector().size() < 2 || !it->getVec2(low_p, low_v)) {
    return Value::undefined.clone();
  }
  high_p = low_p;
  high_v = low_v;

  for (++it; it != vec.end(); ++it) {
    double this_p, this_v;
    if (it->getVec2(this_p, this_v)) {
      if (this_p <= p && (this_p > low_p || low_p > p)) {
        low_p = this_p;
        low_v = this_v;
      }
      if (this_p >= p && (this_p < high_p || high_p < p)) {
        high_p = this_p;
        high_v = this_v;
      }
    }
  }
  if (p <= low_p) return {high_v};
  if (p >= high_p) return {low_v};
  double f = (p - low_p) / (high_p - low_p);
  return {high_v * f + low_v * (1 - f)};
}

/*
   Pattern:

   "search" "(" ( match_value | list_of_match_values ) "," vector_of_vectors
        ("," num_returns_per_match
          ("," index_col_num )? )?
        ")";
   match_value : ( Value::Type::NUMBER | Value::Type::STRING );
   list_of_values : "[" match_value ("," match_value)* "]";
   vector_of_vectors : "[" ("[" Value ("," Value)* "]")+ "]";
   num_returns_per_match : int;
   index_col_num : int;

   The search string and searched strings can be unicode strings.
   Examples:
   Index values return as list:
    search("a","abcdabcd");
        - returns [0]
    search("Л","Л");  //A unicode string
        - returns [0]
    search("🂡aЛ","a🂡Л🂡a🂡Л🂡a",0);
        - returns [[1,3,5,7],[0,4,8],[2,6]]
    search("a","abcdabcd",0); //Search up to all matches
        - returns [[0,4]]
    search("a","abcdabcd",1);
        - returns [0]
    search("e","abcdabcd",1);
        - returns []
    search("a",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ]);
        - returns [0,4]

   Search on different column; return Index values:
    search(3,[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",3] ], 0, 1);
        - returns [0,8]

   Search on list of values:
    Return all matches per search vector element:
      search("abc",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 0);
        - returns [[0,4],[1,5],[2,6]]

    Return first match per search vector element; special case return vector:
      search("abc",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 1);
        - returns [0,1,2]

    Return first two matches per search vector element; vector of vectors:
      search("abce",[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ], 2);
        - returns [[0,4],[1,5],[2,6],[8]]

 */

static VectorType search(const str_utf8_wrapper& find, const str_utf8_wrapper& table,
                         unsigned int num_returns_per_match, EvaluationSession *session)
{
  VectorType returnvec(session);
  // Unicode glyph count for the length
  size_t findThisSize = find.get_utf8_strlen();
  size_t searchTableSize = table.get_utf8_strlen();
  for (size_t i = 0; i < findThisSize; ++i) {
    unsigned int matchCount = 0;
    VectorType resultvec(session);
    const auto ft = find[i];
    for (size_t j = 0; j < searchTableSize; ++j) {
      const auto st = table[j];
      if (!ft.empty() && !st.empty() && ft.get_utf8_char() == st.get_utf8_char()) {
        matchCount++;
        if (num_returns_per_match == 1) {
          returnvec.emplace_back(double(j));
          break;
        } else {
          resultvec.emplace_back(double(j));
        }
        if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) {
          break;
        }
      }
    }
    if (num_returns_per_match == 0 || num_returns_per_match > 1) {
      returnvec.emplace_back(std::move(resultvec));
    }
  }
  return returnvec;
}

static VectorType search(const str_utf8_wrapper& find, const VectorType& table,
                         unsigned int num_returns_per_match, unsigned int index_col_num,
                         const Location& loc, EvaluationSession *session)
{
  VectorType returnvec(session);
  // Unicode glyph count for the length
  unsigned int findThisSize = find.get_utf8_strlen();
  unsigned int searchTableSize = table.size();
  for (size_t i = 0; i < findThisSize; ++i) {
    unsigned int matchCount = 0;
    VectorType resultvec(session);
    const auto ft = find[i];
    for (size_t j = 0; j < searchTableSize; ++j) {
      const auto& entryVec = table[j].toVector();
      if (entryVec.size() <= index_col_num) {
        LOG(message_group::Warning, loc, session->documentRoot(),
            "Invalid entry in search vector at index %1$d, required number of values in the entry: "
            "%2$d. Invalid entry: %3$s",
            j, (index_col_num + 1), table[j].toEchoStringNoThrow());
        return {session};
      }
      const auto& entry = entryVec[index_col_num];
      if (entry.type() != Value::Type::STRING) {
        // Just as "==" silently treats a type mismatch as not-equal, we treat a type mismatch
        // as not matching.
        continue;
      }
      if (!ft.empty() && ft.get_utf8_char() == entry.toStrUtf8Wrapper().get_utf8_char()) {
        matchCount++;
        if (num_returns_per_match == 1) {
          returnvec.emplace_back(double(j));
          break;
        } else {
          resultvec.emplace_back(double(j));
        }
        if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) {
          break;
        }
      }
    }
    if (num_returns_per_match == 0 || num_returns_per_match > 1) {
      returnvec.emplace_back(std::move(resultvec));
    }
  }
  return returnvec;
}

Value builtin_search(Arguments arguments, const Location& loc)
{
  if (arguments.size() < 2 || arguments.size() > 4) {
    print_argCnt_warning("search", arguments.size(), "between 2 and 4", loc, arguments.documentRoot());
    return Value::undefined.clone();
  }

  const Value& findThis = arguments[0].value;
  const Value& searchTable = arguments[1].value;
  unsigned int num_returns_per_match =
    (arguments.size() > 2) ? (unsigned int)arguments[2]->toDouble() : 1;
  unsigned int index_col_num = (arguments.size() > 3) ? (unsigned int)arguments[3]->toDouble() : 0;

  VectorType returnvec(arguments.session());

  if (findThis.type() == Value::Type::NUMBER) {
    unsigned int matchCount = 0;
    size_t j = 0;
    for (const auto& search_element : searchTable.toVector()) {
      if ((index_col_num == 0 && (findThis == search_element).toBool()) ||
          (index_col_num < search_element.toVector().size() &&
           (findThis == search_element.toVector()[index_col_num]).toBool())) {
        returnvec.emplace_back(double(j));
        matchCount++;
        if (num_returns_per_match != 0 && matchCount >= num_returns_per_match) break;
      }
      ++j;
    }
  } else if (findThis.type() == Value::Type::STRING) {
    if (searchTable.type() == Value::Type::STRING) {
      returnvec = search(findThis.toStrUtf8Wrapper(), searchTable.toStrUtf8Wrapper(),
                         num_returns_per_match, arguments.session());
    } else {
      returnvec = search(findThis.toStrUtf8Wrapper(), searchTable.toVector(), num_returns_per_match,
                         index_col_num, loc, arguments.session());
    }
  } else if (findThis.type() == Value::Type::VECTOR) {
    const auto& findVec = findThis.toVector();
    for (const auto& find_value : findVec) {
      unsigned int matchCount = 0;
      VectorType resultvec(arguments.session());

      size_t j = 0;
      for (const auto& search_element : searchTable.toVector()) {
        if ((index_col_num == 0 && (find_value == search_element).toBool()) ||
            (index_col_num < search_element.toVector().size() &&
             (find_value == search_element.toVector()[index_col_num]).toBool())) {
          matchCount++;
          if (num_returns_per_match == 1) {
            returnvec.emplace_back(double(j));
            break;
          } else {
            resultvec.emplace_back(double(j));
          }
          if (num_returns_per_match > 1 && matchCount >= num_returns_per_match) break;
        }
        ++j;
      }
      if ((num_returns_per_match == 1 && matchCount == 0) || num_returns_per_match == 0 ||
          num_returns_per_match > 1) {
        returnvec.emplace_back(std::move(resultvec));
      }
    }
  } else {
    return Value::undefined.clone();
  }
  return std::move(returnvec);
}

Value builtin_version(Arguments arguments, const Location& /*loc*/)
{
  VectorType vec(arguments.session());
  vec.emplace_back(double(openscad_version_year));
  vec.emplace_back(double(openscad_version_month));
  if (openscad_has_day) {
    vec.emplace_back(double(openscad_version_day));
  }
  return std::move(vec);
}

Value builtin_version_num(Arguments arguments, const Location& loc)
{
  Value val =
    (arguments.size() == 0) ? builtin_version(std::move(arguments), loc) : std::move(arguments[0].value);
  double y, m, d;
  if (!val.getVec3(y, m, d, 0)) {
    return Value::undefined.clone();
  }
  return {y * 10000 + m * 100 + d};
}

Value builtin_parent_module(Arguments arguments, const Location& loc)
{
  double d;
  if (arguments.size() == 0) {
    d = 1;
  } else if (!check_arguments("parent_module", arguments, loc, {Value::Type::NUMBER})) {
    return Value::undefined.clone();
  } else {
    d = arguments[0]->toDouble();
  }

  int n = trunc(d);
  int s = UserModule::stack_size();
  if (n < 0) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Negative parent module index (%1$d) not allowed", n);
    return Value::undefined.clone();
  }
  if (n >= s) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Parent module index (%1$d) greater than the number of modules on the stack", n);
    return Value::undefined.clone();
  }
  return {UserModule::stack_element(s - 1 - n)};
}

Value builtin_norm(Arguments arguments, const Location& loc)
{
  if (!check_arguments("norm", arguments, loc, {Value::Type::VECTOR})) {
    return Value::undefined.clone();
  }
  double sum = 0;
  for (const auto& v : arguments[0]->toVector()) {
    if (v.type() == Value::Type::NUMBER) {
      double x = v.toDouble();
      sum += x * x;
    } else {
      LOG(message_group::Warning, loc, arguments.documentRoot(), "Incorrect arguments to norm()");
      return Value::undefined.clone();
    }
  }
  return {sqrt(sum)};
}

Value builtin_cross(Arguments arguments, const Location& loc)
{
  if (!check_arguments("cross", arguments, loc, {Value::Type::VECTOR, Value::Type::VECTOR})) {
    return Value::undefined.clone();
  }

  const auto& v0 = arguments[0]->toVector();
  const auto& v1 = arguments[1]->toVector();
  if ((v0.size() == 2) && (v1.size() == 2)) {
    return {v0[0].toDouble() * v1[1].toDouble() - v0[1].toDouble() * v1[0].toDouble()};
  }

  if ((v0.size() != 3) || (v1.size() != 3)) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Invalid vector size of parameter for cross()");
    return Value::undefined.clone();
  }
  for (unsigned int a = 0; a < 3; ++a) {
    if ((v0[a].type() != Value::Type::NUMBER) || (v1[a].type() != Value::Type::NUMBER)) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Invalid value in parameter vector for cross()");
      return Value::undefined.clone();
    }
    double d0 = v0[a].toDouble();
    double d1 = v1[a].toDouble();
    if (std::isnan(d0) || std::isnan(d1)) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Invalid value (NaN) in parameter vector for cross()");
      return Value::undefined.clone();
    }
    if (std::isinf(d0) || std::isinf(d1)) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Invalid value (INF) in parameter vector for cross()");
      return Value::undefined.clone();
    }
  }

  double x = v0[1].toDouble() * v1[2].toDouble() - v0[2].toDouble() * v1[1].toDouble();
  double y = v0[2].toDouble() * v1[0].toDouble() - v0[0].toDouble() * v1[2].toDouble();
  double z = v0[0].toDouble() * v1[1].toDouble() - v0[1].toDouble() * v1[0].toDouble();

  return VectorType(arguments.session(), x, y, z);
}

namespace spline_helpers {
// interpolator functions take an interpolants Argument of either NUMBER or VECTOR
typedef std::function<double(unsigned)> InterpolantResult_t;
std::function<double(unsigned)> splineInterpolants(const Argument& nArg, double x0, double x1,
                                                   unsigned& nOutputPoints)
{
  auto typeOfn = nArg->type();
  if (typeOfn == Value::Type::NUMBER) {
    // nArg determines the size of the the returned Vector
    auto d = nArg->toDouble();
    if (d < 2) throw std::runtime_error("at least two interpolation points are required");
    nOutputPoints = static_cast<unsigned>(d);
    // equal spaced x values from x0 to x1, inclusive
    if ((x1 == x0) || std::isnan(x0) || std::isnan(x1))
      throw std::runtime_error("The range of x cannot be degenerate");
    auto xrange = x1 - x0;
    return [x0, xrange, nOutputPoints](unsigned i) { return x0 + (xrange * i) / (nOutputPoints - 1); };
  } else if (typeOfn == Value::Type::VECTOR) {
    // caller provided a list of explicit x values
    const auto& xValues = nArg->toVector();
    nOutputPoints = xValues.size();
    return [&xValues](unsigned i) {
      const auto& val = xValues[i];
      if (val.type() != Value::Type::NUMBER) throw std::runtime_error("Non-numeric x value to spline");
      return val.toDouble();
    };
  } else throw std::runtime_error("The interpolants parameter must be a number or a vector");
}

typedef std::function<void(unsigned, double[])> xyFcn_t;

// interpolator functions return a vector of [x,y] points
Value splineResult(EvaluationSession *session, unsigned nOutputPoints, const xyFcn_t& xyFcn,
                   unsigned dim = 2)
{
  VectorType result(session);
  result.reserve(nOutputPoints);
  for (unsigned i = 0; i < nOutputPoints; i++) {
    std::shared_ptr<double[]> vals(new double[dim]);
    xyFcn(i, &vals[0]);
    VectorType point(session);
    point.reserve(dim);
    for (unsigned i = 0; i < dim; i++) point.emplace_back(vals[i]);
    result.emplace_back(std::move(point));
  }
  return std::move(result);  // only successful return
}

namespace catmull_rom {
// The catmull_rom built in supports both 2D and 3D splining.

struct InterpolantArg { // specifying interpolants for catmull_rom...
  unsigned nOutputPoints; // number of points requested to interpolate
  // only makes sense as indicies into control points
  unsigned startIndex;
  unsigned stopIndex;
  InterpolantArg() : nOutputPoints(0), startIndex(0), stopIndex(0) {}
};
typedef std::vector<InterpolantArg> InterpolantArgsType;

// The boost catmull_rom class is templated on its point type, 
// which must have its dimensionality known at construction
template <unsigned NDIM, class PointContainer>
Value evaluate(EvaluationSession *session, PointContainer& points,
               const InterpolantArgsType& interpolants, bool closed, double alpha, bool tangents)
{
  typedef std::array<double, NDIM> Point_t;
  auto numControlPoints = points.size();  // grab before std::move
  boost::math::catmull_rom<Point_t> spline(std::move(points), closed, alpha);

  auto maxParam = spline.max_parameter();
  double minParam = 0;
  unsigned nOutputPoints(0);
  std::vector<double> all_s;

  // convert the incoming InterpolantArgsType, which is indicies into the PointContainer,
  // into actual s values:
  //
  // nOutputPoints is the number of points to output,
  //        except for two consectutive InterpolantArg's in the vector where the second
  //        starts exactly where the first finished. That overlapping point is not duplicated
  //        in the output, therefore nOutputPoints is reduced by one from the total
  //
  //  startIndex and stopIndex have to be distinct (unless nOutputPoints is less than 2)
  //  The maximum for each is either numControlPoints-1 for a closed curve,
  //  or numControlPoints, in which case the interval includes the closing from the last
  //  control back to the first.

  unsigned maxIdx = closed ? numControlPoints : numControlPoints - 1;
  unsigned priorStopIdx = static_cast<unsigned>(-1);
  auto sMax = spline.max_parameter();
  unsigned resultCount = 0;
  for (auto const& i : interpolants) {
    if (i.startIndex >= maxIdx)
      throw std::runtime_error("interpolant start must be less than last control point");
    if (i.stopIndex < i.startIndex || (i.stopIndex == i.startIndex && i.nOutputPoints > 1))
      throw std::runtime_error("interpolant stop point must be beyond start");
    if (i.stopIndex > maxIdx)
      throw std::runtime_error("interpolant stop cannot be beyond last control point");

    auto startS = spline.parameter_at_point(i.startIndex);
    if (i.nOutputPoints == 1) {
      all_s.push_back(startS);
      priorStopIdx = i.startIndex;
      continue;
    }
    auto stopS = i.stopIndex == maxIdx ? spline.max_parameter() : spline.parameter_at_point(i.stopIndex);
    auto rangeS = stopS - startS;
    unsigned c = 0;
    if (i.startIndex == priorStopIdx) c = 1; // special case: skip the first if previous ended here
    for (; c < i.nOutputPoints; c++) {
      // the interpolator with throw if our floating arith manages to exceed sMax
      double s = std::min(sMax, startS + rangeS * static_cast<double>(c) / (i.nOutputPoints - 1));
      all_s.push_back(s);
    }
    priorStopIdx = i.stopIndex;
  }
  xyFcn_t xyFcn;
  if (!tangents) {
    xyFcn = [&spline, &all_s](unsigned i, double xy[]) {
      auto val = spline(all_s[i]);
      for (unsigned j = 0; j < NDIM; j++) xy[j] = val[j];
    };
    resultCount = all_s.size();
  } else {  // intersperse tangents
    xyFcn = [&spline, &all_s](unsigned i, double xy[]) {
      unsigned j = i >> 1;
      Point_t val({});
      auto s = all_s[j];
      if (!(i & 1)) val = spline(s);
      else if (s != spline.max_parameter()) // there is no tangent here
        val = spline.prime(s);
      for (unsigned k = 0; k < NDIM; k++) xy[k] = val[k];
    };
    resultCount = 2 * all_s.size();
  }
  return splineResult(session, resultCount, xyFcn, NDIM);
}
}  // namespace catmull_rom
}  // namespace spline_helpers

Value builtin_cubic_spline(Arguments arguments, const Location& loc)
{ /* A wrapper for boost's cardinal_cubic_b_spline  */
  if (arguments.size() < 4) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "Too few arguments to cubic_spline()");
    return Value::undefined.clone();
  } else if (arguments.size() > 6) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "Too many arguments to cubic_spline()");
    return Value::undefined.clone();
  }

  // first argument
  const auto& yvalues = arguments[0]->toVector();
  if (yvalues.size() < 2) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Minimum size of 2 for control values vector in cubic_spline()");
    return Value::undefined.clone();
  }

  // The x0,x1 args must be NUMBER
  for (unsigned argc = 2; argc <= 3; argc++)
    if (arguments[argc]->type() != Value::Type::NUMBER) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "x0 and x1 must be numbers in cubic_spline()");
      return Value::undefined.clone();
    }

  double x0 = arguments[2]->toDouble();
  double x1 = arguments[3]->toDouble();
    
  // second argument, interpolants, can either be a number or a list of x values
  unsigned nOutputPoints(0);
  spline_helpers::InterpolantResult_t xFcn;
  try {
    xFcn = spline_helpers::splineInterpolants(arguments[1], x0, x1, nOutputPoints);
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        std::string(e.what()) + " in cubic_spline()");
    return Value::undefined.clone();
  }

  // slopes at left endpoint and right endpoint defaults. Init to boost library defaults
  double le(std::numeric_limits<double>::quiet_NaN()), re(std::numeric_limits<double>::quiet_NaN());
  if (arguments.size() > 4) {
    if (arguments[4]->isDefined())
      le = arguments[4]->toDouble();
    if ((arguments.size() > 5) && arguments[5]->isDefined())
      re = arguments[5]->toDouble();
  }

  class spline_iterator  // wrap a boost BidiIterator around VectorType
  {
  private:
    const VectorType& v;  // VectorType of NUMBER
    unsigned idx;

  public:
    spline_iterator(const VectorType& v, bool atBegin = true) : v(v), idx(atBegin ? 0 : v.size()) {}
    // boost BidiIterator requires these two operators:
    int operator-(const spline_iterator& other) const { return idx - other.idx; }
    double operator[](unsigned id) const
    {
      const auto& val = v[id];
      if (val.type() != Value::Type::NUMBER)
        throw std::runtime_error("Non-numeric value to cubic_spline");
      return val.toDouble();
    }
  };

  double step = (x1 - x0) / (yvalues.size() - 1);
  spline_iterator begin(yvalues);
  spline_iterator end(yvalues, false);
  try {
    // after processing all the openscad arguments, we (finally) can calculate
    boost::math::interpolators::cardinal_cubic_b_spline<double> spline(begin, end, x0, step, le, re);
    auto xyFcn = [&xFcn, &spline](unsigned i, double xy[]) {
      auto x = xy[0] = xFcn(i);
      xy[1] = spline(x);
    };
    return spline_helpers::splineResult(arguments.session(), nOutputPoints, xyFcn);
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        std::string(e.what()) + std::string("for cubic_spline"));
  }
  return Value::undefined.clone();
}

Value builtin_makima_spline(Arguments arguments, const Location& loc)
{ /* A wrapper for boost's modified Akima spline
  ** Compared to cubic spline above, makima does not require equal spaced x control points,
  ** but does require a minimum of 4 compared to just 2 for cubic.
  ** Both allow the caller to specify the first derivative at the begin and/or end of the x range.
  */
  if (arguments.size() < 2) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "Too few arguments to makima_spline()");
    return Value::undefined.clone();
  } else if (arguments.size() > 4) {
    LOG(message_group::Warning, loc, arguments.documentRoot(), "Too many arguments to makima_spline()");
    return Value::undefined.clone();
  }

  const auto& xyArg =  // first argument
    arguments[0];      // input list of control points, [[x0,y0], [x1,y1], ... 4 minimum per makima
  bool ok = xyArg->type() == Value::Type::VECTOR;
  if (ok) {
    const auto& xyv = xyArg->toVector();
    static const unsigned MAKIMA_MIN_CONTROL_POINTS(4);  // per boost modified Akima documentation
    if (xyv.size() < MAKIMA_MIN_CONTROL_POINTS) ok = false;
    else {  // Check the only first point dimension and types here. If later points are bogus,
            // will throw exception when calculating the spline below
      const auto& firstPoint = xyv[0];
      if (firstPoint.type() != Value::Type::VECTOR) ok = false;
      else {
        const auto& firstPointV = firstPoint.toVector();
        if (firstPointV.size() != 2) ok = false;
        else if ((firstPointV[0].type() != Value::Type::NUMBER) ||
                 (firstPointV[1].type() != Value::Type::NUMBER))
          ok = false;
      }
    }
  }
  if (!ok) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "First argument must be a list of at least 4 controlling [x,y] points in makima_spline()");
    return Value::undefined.clone();
  }

  const auto& xycontrols = xyArg->toVector();
  // for boost's cubic spline above, it was reasonable to wrap openscad's Vector in a custom iterator
  // and avoid copying the data. For boost's makima instead requires a "container"
  // which const Value::VectorType is not. Therefore we copy the control points into std::vector
  std::vector<double> x, y;
  double x0(std::numeric_limits<double>::max()), x1(std::numeric_limits<double>::lowest());
  for (const auto& p : xycontrols) {
    if (p.type() != Value::Type::VECTOR) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Found a non-point entry in first argument in makima_spline()");
      return Value::undefined.clone();
    }
    auto const& pv = p.toVector();
    if (pv.size() != 2) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Found degenerate point in first argument in makima_spline()");
      return Value::undefined.clone();
    }
    auto d = pv[0].toDouble();
    x.push_back(d);
    if (d < x0) x0 = d;
    if (d > x1) x1 = d;
    y.push_back(pv[1].toDouble());
  }

  // second argument, interpolants
  unsigned nOutputPoints(0);
  const auto &interpolantsArg(arguments[1]);
  spline_helpers::InterpolantResult_t xFcn;
  try {
    xFcn = spline_helpers::splineInterpolants(interpolantsArg, x0, x1, nOutputPoints);
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        std::string(e.what()) + " in makima_spline()");
    return Value::undefined.clone();
  }

  // slopes at left endpoint and right endpoint defaults. Init to boost library defaults
  double le(std::numeric_limits<double>::quiet_NaN()), re(std::numeric_limits<double>::quiet_NaN());
  if (arguments.size() > 2) {
    if (arguments[2]->isDefined())
      le = arguments[2]->toDouble();
    if ((arguments.size() > 3) && arguments[3]->isDefined())
      re = arguments[3]->toDouble();
  }

  try {  // after processing all the openscad arguments, we (finally) can calculate
    boost::math::interpolators::makima spline(std::move(x), std::move(y), le, re);
    auto xyFcn = [&xFcn, &spline](unsigned i, double  xy[]) {
      auto x = xy[0] = xFcn(i);
      xy[1] = spline(x);
    };
    return spline_helpers::splineResult(arguments.session(), nOutputPoints, xyFcn);
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        std::string(e.what()) + std::string("for makima_spline"));
  }
  return Value::undefined.clone();
}

Value builtin_catmull_rom_spline(Arguments arguments, const Location& loc)
{
  /* A wrapper for boost's catmull_rom spline
  ** Its input is a list of control points that are ordered as a path in [x,y].
  ** The result can be requested to be open or closed
  */
  if (arguments.size() < 2) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Too few arguments to catmull_rom_spline()");
    return Value::undefined.clone();
  } else if (arguments.size() > 5) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Too many arguments to catmull_rom_spline()");
    return Value::undefined.clone();
  }

  // copy all the input [x,y] points
  typedef std::array<double, 3> Point3D_t;  // boost catmull_rom needs points
  typedef std::array<double, 2> Point2D_t;  // Ugly to have 2 classes but..
  // the boost documentation says std::vector has all the right signatures for catmull_rom....
  // ...and it does compile OK. But at run time, the splining crashes
  // because the boost templated class calls the default constructor of its Point and
  // expects the Point to have the right size. std::vector's default is size 0.

  const auto& xyArg =  // first argument
    arguments[0];      // input list of control points, [[x0,y0], [x1,y1],
  if (xyArg->type() != Value::Type::VECTOR) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "First argument must be a vector in catmull_rom_spline()");
    return Value::undefined.clone();
  }

  const auto& xycontrols = xyArg->toVector();
  std::vector<Point3D_t> points3D;  // fill one or the other
  std::vector<Point2D_t> points2D;
  unsigned ndim = 0;
  for (const auto& p : xycontrols) {
    if (p.type() != Value::Type::VECTOR) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Found a non-point entry in first argument in catmull_rom_spline()");
      return Value::undefined.clone();
    }
    auto const& pv = p.toVector();
    if (ndim != 0) {
      if (pv.size() != ndim) {
        LOG(message_group::Warning, loc, arguments.documentRoot(),
            "All the points must have the same dimension in catmull_rom_spline()");
        return Value::undefined.clone();
      }
    } else ndim = pv.size();
    if ((pv.size() < 2) || (pv.size() > 3)) {
      LOG(message_group::Warning, loc, arguments.documentRoot(),
          "Found degenerate point in first argument in catmull_rom_spline()");
      return Value::undefined.clone();
    }
    if (ndim == 2) {
      Point2D_t pxy({});
      for (unsigned i = 0; i < pv.size(); i++) pxy[i] = pv[i].toDouble();
      points2D.push_back(pxy);
    } else if (ndim == 3) {
      Point3D_t pxy({});
      for (unsigned i = 0; i < pv.size(); i++) pxy[i] = pv[i].toDouble();
      points3D.push_back(pxy);
    }
  }

  auto numControlPoints = std::max(points2D.size(), points3D.size());

  // process args out of order cuz we need the flags to deal with interpolants
  bool closed = false;
  double alpha = 1.0 / 2.0;
  bool intersperseTangents = false;
  if (arguments.size() > 2) {
    if (arguments[2]->isDefined())
      closed = arguments[2]->toBool();
    if (arguments.size() > 3) {
      if (arguments[3]->isDefined())
        alpha = arguments[3]->toDouble();
      if ((arguments.size() > 4 ) && arguments[4]->isDefined()) 
        intersperseTangents = arguments[4]->toBool();
    }
  }

  // second argument, interpolants, can either be
  //    a number or
  //    a list of s lookup indicies among control points, or...
  // ...a list of lists
  // "s" is the catmull_rom parameterization value
  const auto& interpolantsArg(arguments[1]);
  spline_helpers::catmull_rom::InterpolantArgsType interpolants;
  if (interpolantsArg->type() ==
      Value::Type::NUMBER) {  // just a number. interpolate through all control points
    interpolants.resize(1);
    auto& oneArg = interpolants.front();
    oneArg.nOutputPoints = static_cast<unsigned>(interpolantsArg->toInteger());
    oneArg.startIndex = 0;
    oneArg.stopIndex = numControlPoints - (closed ? 0 : 1);
  } else if (interpolantsArg->type() == Value::Type::VECTOR) {
    auto const& interpolants0Vec = interpolantsArg->toVector();
    bool isNumbers = false;
    bool isVectors = false;
    for (unsigned i = 0; i < interpolants0Vec.size(); i++) {
      auto const& p = interpolants0Vec[i];
      if (p.type() == Value::Type::VECTOR) {  // a list of lists. Each list item must be size 3
        if (i == 0) isVectors = true;
        const auto& interpolants1Vec = p.toVector();
        if (isVectors && interpolants1Vec.size() == 3) {
          const auto& m0 = interpolants1Vec[0];
          const auto& m1 = interpolants1Vec[1];
          const auto& m2 = interpolants1Vec[2];
          spline_helpers::catmull_rom::InterpolantArg arg;
          arg.nOutputPoints = static_cast<unsigned>(m0.toInteger());
          arg.startIndex = static_cast<unsigned>(m1.toInteger());
          arg.stopIndex = static_cast<unsigned>(m2.toInteger());
          if (arg.nOutputPoints) // no output, no problem
            interpolants.push_back(arg);
        } else {
          LOG(message_group::Warning, loc, arguments.documentRoot(),
              "interpolant list of list not size 3 in catmull_rom_spline()");
          return Value::undefined.clone();
        }
      } else if (p.type() == Value::Type::NUMBER) {
        if (i == 0) {
          isNumbers = true;
          interpolants.resize(1);
          interpolants.front().stopIndex = numControlPoints - 1;
        }
        if (!isNumbers) {
          LOG(message_group::Warning, loc, arguments.documentRoot(),
              "interpolant invalid in catmull_rom_spline()");
          return Value::undefined.clone();
        }
        switch (i) {
        case 0:  interpolants.front().nOutputPoints = static_cast<unsigned>(p.toInteger()); break;
        case 1:  interpolants.front().startIndex = static_cast<unsigned>(p.toInteger()); break;
        case 2:  interpolants.front().stopIndex = static_cast<unsigned>(p.toInteger()); break;
        default: {
          LOG(message_group::Warning, loc, arguments.documentRoot(),
              "interpolant list max size is 3 in catmull_rom_spline()");
          return Value::undefined.clone();
        }
        }
      } else {
        LOG(message_group::Warning, loc, arguments.documentRoot(),
            "Wrong datatype interpolants in catmull_rom_spline()");
        return Value::undefined.clone();
      }
    }
  } else {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        "Wrong datatype interpolants in catmull_rom_spline()");
    return Value::undefined.clone();
  }

  try {  // after processing all the openscad arguments, we (finally) can calculate
    if (ndim == 2)
      return spline_helpers::catmull_rom::evaluate<2, std::vector<Point2D_t>>(
        arguments.session(), points2D, interpolants, closed, alpha, intersperseTangents);
    else if (ndim == 3)
      return spline_helpers::catmull_rom::evaluate<3, std::vector<Point3D_t>>(
        arguments.session(), points3D, interpolants, closed, alpha, intersperseTangents);
  } catch (const std::exception& e) {
    LOG(message_group::Warning, loc, arguments.documentRoot(),
        std::string(e.what()) + std::string(" for catmull_rom_spline"));
  }
  return Value::undefined.clone();
}

Value builtin_textmetrics(Arguments arguments, const Location& loc)
{
  auto *session = arguments.session();
  Parameters parameters =
    Parameters::parse(std::move(arguments), loc, {"text", "size", "font"},
                      {"direction", "language", "script", "halign", "valign", "spacing", "em"});
  parameters.set_caller("textmetrics");

  FreetypeRenderer::Params ftparams(parameters);
  ftparams.set_loc(loc);
  ftparams.set_documentPath(session->documentRoot());
  ftparams.detect_properties();

  FreetypeRenderer::TextMetrics metrics(ftparams);
  if (!metrics.ok) {
    return Value::undefined.clone();
  }

  // The bounding box, ascent/descent, and offset values will be zero
  // if the text consists of nothing but whitespace.
  VectorType bbox_pos(session);
  bbox_pos.reserve(2);
  bbox_pos.emplace_back(metrics.bbox_x);
  bbox_pos.emplace_back(metrics.bbox_y);

  VectorType bbox_dims(session);
  bbox_dims.reserve(2);
  bbox_dims.emplace_back(metrics.bbox_w);
  bbox_dims.emplace_back(metrics.bbox_h);

  VectorType offset(session);
  offset.reserve(2);
  offset.emplace_back(metrics.x_offset);
  offset.emplace_back(metrics.y_offset);

  // The advance values are valid whether or not the text
  // is whitespace.
  VectorType advance(session);
  advance.reserve(2);
  advance.emplace_back(metrics.advance_x);
  advance.emplace_back(metrics.advance_y);

  ObjectType text_metrics(session);
  text_metrics.set("position", std::move(bbox_pos));
  text_metrics.set("size", std::move(bbox_dims));
  text_metrics.set("ascent", metrics.ascent);
  text_metrics.set("descent", metrics.descent);
  text_metrics.set("offset", std::move(offset));
  text_metrics.set("advance", std::move(advance));
  return std::move(text_metrics);
}

Value builtin_fontmetrics(Arguments arguments, const Location& loc)
{
  auto *session = arguments.session();
  Parameters parameters = Parameters::parse(std::move(arguments), loc, {"size", "font", "em"});
  parameters.set_caller("fontmetrics");

  FreetypeRenderer::Params ftparams(parameters);
  ftparams.set_loc(loc);
  ftparams.set_documentPath(session->documentRoot());
  ftparams.detect_properties();

  FreetypeRenderer::FontMetrics metrics(ftparams);
  if (!metrics.ok) {
    return Value::undefined.clone();
  }

  ObjectType nominal(session);
  nominal.set("ascent", metrics.nominal_ascent);
  nominal.set("descent", metrics.nominal_descent);

  ObjectType max(session);
  max.set("ascent", metrics.max_ascent);
  max.set("descent", metrics.max_descent);

  ObjectType font(session);
  font.set("family", metrics.family_name);
  font.set("style", metrics.style_name);

  ObjectType font_metrics(session);
  font_metrics.set("nominal", nominal);
  font_metrics.set("max", max);
  font_metrics.set("interline", metrics.interline);
  font_metrics.set("font", font);

  return std::move(font_metrics);
}

Value builtin_is_undef(const std::shared_ptr<const Context>& context, const FunctionCall *call)
{
  if (call->arguments.size() != 1) {
    print_argCnt_warning("is_undef", call->arguments.size(), "1", call->location(),
                         context->documentRoot());
    return Value::undefined.clone();
  }
  if (auto lookup = std::dynamic_pointer_cast<Lookup>(call->arguments[0]->getExpr())) {
    auto result = context->try_lookup_variable(lookup->get_name());
    return !result || result->isUndefined();
  } else {
    return call->arguments[0]->getExpr()->evaluate(context).isUndefined();
  }
}

Value builtin_is_list(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_list", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::VECTOR)};
}

Value builtin_is_num(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_num", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::NUMBER) && !std::isnan(arguments[0]->toDouble())};
}

Value builtin_is_bool(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_bool", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::BOOL)};
}

Value builtin_is_string(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_string", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::STRING)};
}

Value builtin_is_function(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_function", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::FUNCTION)};
}

Value builtin_is_object(Arguments arguments, const Location& loc)
{
  if (!check_arguments("is_object", arguments, loc, 1)) {
    return Value::undefined.clone();
  }
  return {arguments[0]->isDefinedAs(Value::Type::OBJECT)};
}

Value builtin_import(Arguments arguments, const Location& loc)
{
  auto session = arguments.session();
  const Parameters parameters = Parameters::parse(std::move(arguments), loc, {}, {"file"});
  std::string raw_filename = parameters.get("file", "");
  std::string file =
    lookup_file(raw_filename, loc.filePath().parent_path().string(), parameters.documentRoot());
  return import_json(file, session, loc);
}

void register_builtin_functions()
{
  Builtins::init("abs", new BuiltinFunction(&builtin_abs),
                 {
                   "abs(number) -> number",
                 });

  Builtins::init("sign", new BuiltinFunction(&builtin_sign),
                 {
                   "sign(number) -> -1, 0 or 1",
                 });

  Builtins::init("rands", new BuiltinFunction(&builtin_rands),
                 {
                   "rands(min, max, num_results) -> vector",
                   "rands(min, max, num_results, seed) -> vector",
                 });

  Builtins::init("min", new BuiltinFunction(&builtin_min),
                 {
                   "min(number, number, ...) -> number",
                   "min(vector) -> number",
                 });

  Builtins::init("max", new BuiltinFunction(&builtin_max),
                 {
                   "max(number, number, ...) -> number",
                   "max(vector) -> number",
                 });

  Builtins::init("sin", new BuiltinFunction(&builtin_sin),
                 {
                   "sin(degrees) -> number",
                 });

  Builtins::init("cos", new BuiltinFunction(&builtin_cos),
                 {
                   "cos(degrees) -> number",
                 });

  Builtins::init("asin", new BuiltinFunction(&builtin_asin),
                 {
                   "asin(number) -> degrees",
                 });

  Builtins::init("acos", new BuiltinFunction(&builtin_acos),
                 {
                   "acos(number) -> degrees",
                 });

  Builtins::init("tan", new BuiltinFunction(&builtin_tan),
                 {
                   "tan(degrees) -> number",
                 });

  Builtins::init("atan", new BuiltinFunction(&builtin_atan),
                 {
                   "atan(number) -> degrees",
                 });

  Builtins::init("atan2", new BuiltinFunction(&builtin_atan2),
                 {
                   "atan2(number, number) -> degrees",
                 });

  Builtins::init("round", new BuiltinFunction(&builtin_round),
                 {
                   "round(number) -> number",
                 });

  Builtins::init("ceil", new BuiltinFunction(&builtin_ceil),
                 {
                   "ceil(number) -> number",
                 });

  Builtins::init("floor", new BuiltinFunction(&builtin_floor),
                 {
                   "floor(number) -> number",
                 });

  Builtins::init("pow", new BuiltinFunction(&builtin_pow),
                 {
                   "pow(base, exponent) -> number",
                 });

  Builtins::init("sqrt", new BuiltinFunction(&builtin_sqrt),
                 {
                   "sqrt(number) -> number",
                 });

  Builtins::init("exp", new BuiltinFunction(&builtin_exp),
                 {
                   "exp(number) -> number",
                 });

  Builtins::init("len", new BuiltinFunction(&builtin_length),
                 {
                   "len(string) -> number",
                   "len(vector) -> number",
                 });

  Builtins::init("log", new BuiltinFunction(&builtin_log),
                 {
                   "log(number) -> number",
                 });

  Builtins::init("ln", new BuiltinFunction(&builtin_ln),
                 {
                   "ln(number) -> number",
                 });

  Builtins::init("str", new BuiltinFunction(&builtin_str),
                 {
                   "str(number or string, ...) -> string",
                 });

  Builtins::init("chr", new BuiltinFunction(&builtin_chr),
                 {
                   "chr(number) -> string",
                   "chr(vector) -> string",
                   "chr(range) -> string",
                 });

  Builtins::init("textmetrics",
                 new BuiltinFunction(&builtin_textmetrics, &Feature::ExperimentalTextMetricsFunctions),
                 {
                   "textmetrics(text, size, font, direction, language, script, halign, valign, spacing, "
                   "em) -> object",
                 });

  Builtins::init("fontmetrics",
                 new BuiltinFunction(&builtin_fontmetrics, &Feature::ExperimentalTextMetricsFunctions),
                 {
                   "fontmetrics(size, font, em) -> object",
                 });

  Builtins::init("ord", new BuiltinFunction(&builtin_ord),
                 {
                   "ord(string) -> number",
                 });

  Builtins::init("concat", new BuiltinFunction(&builtin_concat),
                 {
                   "concat(number or string or vector, ...) -> vector",
                 });

  Builtins::init("lookup", new BuiltinFunction(&builtin_lookup),
                 {
                   "lookup(key, <key,value> vector) -> value",
                 });

  Builtins::init(
    "search", new BuiltinFunction(&builtin_search),
    {
      "search(string , string or vector [, num_returns_per_match [, index_col_num ] ] ) -> vector",
    });

  Builtins::init("version", new BuiltinFunction(&builtin_version),
                 {
                   "version() -> vector",
                 });

  Builtins::init("version_num", new BuiltinFunction(&builtin_version_num),
                 {
                   "version_num() -> number",
                 });

  Builtins::init("norm", new BuiltinFunction(&builtin_norm),
                 {
                   "norm(vector) -> number",
                 });

  Builtins::init("cross", new BuiltinFunction(&builtin_cross),
                 {
                   "cross(vector, vector) -> vector",
                 });

  Builtins::init("parent_module", new BuiltinFunction(&builtin_parent_module),
                 {
                   "parent_module(number) -> string",
                 });

  Builtins::init("is_undef", new BuiltinFunction(&builtin_is_undef),
                 {
                   "is_undef(arg) -> boolean",
                 });

  Builtins::init("is_list", new BuiltinFunction(&builtin_is_list),
                 {
                   "is_list(arg) -> boolean",
                 });

  Builtins::init("is_num", new BuiltinFunction(&builtin_is_num),
                 {
                   "is_num(arg) -> boolean",
                 });

  Builtins::init("is_bool", new BuiltinFunction(&builtin_is_bool),
                 {
                   "is_bool(arg) -> boolean",
                 });

  Builtins::init("is_string", new BuiltinFunction(&builtin_is_string),
                 {
                   "is_string(arg) -> boolean",
                 });

  Builtins::init("is_function", new BuiltinFunction(&builtin_is_function),
                 {
                   "is_function(arg) -> boolean",
                 });

  Builtins::init("is_object",
                 new BuiltinFunction(&builtin_is_object, &Feature::ExperimentalTextMetricsFunctions),
                 {
                   "is_object(arg) -> boolean",
                 });

  Builtins::init("object", new BuiltinFunction(&builtin_object, &Feature::ExperimentalObjectFunction),
                 {
                   "object([ object, ] [ key-val list, ] key=value, ...) -> object",
                 });

  Builtins::init("has_key", new BuiltinFunction(&builtin_has_key, &Feature::ExperimentalObjectFunction),
                 {
                   "has_key(object, key) -> boolean",
                 });

  Builtins::init("import", new BuiltinFunction(&builtin_import, &Feature::ExperimentalImportFunction),
                 {
                   "import(file) -> object",
                 });

  Builtins::init("cubic_spline", new BuiltinFunction(&builtin_cubic_spline),
                 {
                   "cubic_spline(controls, interpolants, x0, x1) -> vector",
                   "cubic_spline(controls, interpolants, x0, x1, s0) -> vector",
                   "cubic_spline(controls, interpolants, x0, x1, s0, s1) -> vector",
                 });
  Builtins::init("makima_spline", new BuiltinFunction(&builtin_makima_spline),
                 {
                   "makima_spline(controls, interpolants) -> vector",
                   "makima_spline(controls, interpolants, s0) -> vector",
                   "makima_spline(controls, interpolants, s0, s1) -> vector",
                 });
  Builtins::init("catmull_rom_spline", new BuiltinFunction(&builtin_catmull_rom_spline),
                 {
                   "catmull_rom_spline(controls, interpolants) -> vector",
                   "catmull_rom_spline(controls, interpolants, closed) -> vector",
                   "catmull_rom_spline(controls, interpolants, closed, alpha) -> vector",
                   "catmull_rom_spline(controls, interpolants, closed, alpha, tangents) -> vector",
                 });
}
