#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include <libintl.h>
// Undefine some defines from libintl.h to presolve
// some collisions in boost headers later
#if defined snprintf
#undef snprintf
#endif
#if defined vsnprintf
#undef vsnprintf
#endif

#include <clocale>
#include "core/AST.h"
#include <set>

// It seems standard practice to use underscore for gettext, even though it is reserved.
// Not wanting to risk breaking translations by changing every usage of this,
// I've opted to just disable the check in this case. - Hans L
// NOLINTBEGIN(bugprone-reserved-identifier)
inline char *_(const char *msgid) { return gettext(msgid); }
inline const char *_(const char *msgid, const char *msgctxt) {
  /* The separator between msgctxt and msgid in a .mo file.  */
  const char *GETTEXT_CONTEXT_GLUE = "\004";

  std::string str = msgctxt;
  str += GETTEXT_CONTEXT_GLUE;
  str += msgid;
  auto translation = dcgettext(nullptr, str.c_str(), LC_MESSAGES);
  if (translation == str) {
    return gettext(msgid);
  } else {
    return translation;
  }
}
// NOLINTEND(bugprone-reserved-identifier)

enum class message_group {
  NONE, Error, Warning, UI_Warning, Font_Warning, Export_Warning, Export_Error, UI_Error, Parser_Error, Trace, Deprecated, Echo
};


std::string getGroupName(const enum message_group& group);
std::string getGroupColor(const enum message_group& group);
bool getGroupTextPlain(const enum message_group& group);

struct Message {
  std::string msg;
  Location loc;
  std::string docPath;
  enum message_group group;

  Message()
    : msg(""), loc(Location::NONE), docPath(""), group(message_group::NONE)
  {
  }

  Message(std::string msg, message_group group = message_group::NONE, Location loc = Location::NONE, std::string docPath = "")
    : msg(std::move(msg)), loc(std::move(loc)), docPath(std::move(docPath)), group(group)
  {
  }

  [[nodiscard]] std::string str() const {
    const auto g = group == message_group::NONE ? "" : getGroupName(group) + ": ";
    const auto l = loc.isNone() ? "" : " " + loc.toRelativeString(docPath);
    return g + msg + l;
  }
};

using OutputHandlerFunc = void (const Message&, void *);
using OutputHandlerFunc2 = void (const Message&, void *);

extern OutputHandlerFunc *outputhandler;
extern void *outputhandler_data;

namespace OpenSCAD {
extern std::string debug;
extern bool quiet;
extern bool hardwarnings;
extern int traceDepth;
extern bool traceUsermoduleParameters;
extern bool parameterCheck;
extern bool rangeCheck;
}

void set_output_handler(OutputHandlerFunc *newhandler, OutputHandlerFunc2 *newhandler2, void *userdata);
void no_exceptions_for_warnings();
bool would_have_thrown();

extern std::list<std::string> print_messages_stack;
void print_messages_push();
void print_messages_pop();
void resetSuppressedMessages();


/* PRINT statements come out in same window as ECHO.
   usage: PRINTB("Var1: %s Var2: %i", var1 % var2 ); */
void PRINT(const Message& msgObj);

void PRINT_NOCACHE(const Message& msgObj);
#define PRINTB_NOCACHE(_fmt, _arg) do { } while (0)
// #define PRINTB_NOCACHE(_fmt, _arg) do { PRINT_NOCACHE(str(boost::format(_fmt) % _arg)); } while (0)

/*PRINTD: debugging/verbose output. Usage in code:
   CGAL_Point_3 p0(0,0,0),p1(1,0,0),p2(0,1,0);
   PRINTD(" Created 3 points: ");
   PRINTDB("point0, point1, point2: %s %s %s", p0 % p1 % p2 );
   Usage on command line:
   openscad x.scad --debug=all       # prints all debug messages
   openscad x.scad --debug=<srcfile> # prints only debug msgs from srcfile.*.cc
   (example: openscad --debug=export # prints only debug msgs from export.cc )

   For a debug with heavy computation cost, you can guard so that the computation
   only occurs when debugging is turned on. For example:
   if (OpenSCAD::debug!="") PRINTDB("PolySet dump: %s",ps->dump());
 */

void PRINTDEBUG(const std::string& filename, const std::string& msg);
// NOLINTBEGIN
#define PRINTD(_arg) do { PRINTDEBUG(std::string(__FILE__), _arg); } while (0)
#define PRINTDB(_fmt, _arg) do { try { PRINTDEBUG(std::string(__FILE__), str(boost::format(_fmt) % _arg)); } catch (const boost::io::format_error& e) { PRINTDEBUG(std::string(__FILE__), "bad PRINTDB usage"); } } while (0)
// NOLINTEND

std::string two_digit_exp_format(std::string doublestr);
std::string two_digit_exp_format(double x);
const std::string& quoted_string(const std::string& str);

// extremely simple logging, eventually replace with something like boost.log
// usage: logstream out(5); openscad_loglevel=6; out << "hi";
static int openscad_loglevel = 0;
class logstream
{
public:
  std::ostream *out;
  int loglevel;
  logstream(int level = 0) {
    loglevel = level;
    out = &(std::cout);
  }
  template <typename T> logstream& operator<<(T const& t) {
    if (out && loglevel <= openscad_loglevel) {
      (*out) << t;
      out->flush();
    }
    return *this;
  }
};

inline std::string STR(std::ostringstream& oss) {
  auto s = oss.str();
  oss.str("");  // clear the string buffer for next STR call
  oss.clear();  // reset stream error state for next STR call
  return s;
}

template <typename T, typename ... Args>
std::string STR(std::ostringstream& oss, T&& t, Args&& ... args) {
  oss << t;
  return STR(oss, std::forward<Args>(args)...);
}

template <typename T, typename ... Args>
std::string STR(T&& t, Args&& ... args) {
  // using thread_local here so that recursive template does not instantiate excessive ostringstreams
  thread_local std::ostringstream oss;
  oss << t;
  return STR(oss, std::forward<Args>(args)...);
}

template <typename ... Ts>
class MessageClass
{
private:
  std::string fmt;
  std::tuple<Ts...> args;
  template <std::size_t... Is>
  [[nodiscard]] std::string format(const std::index_sequence<Is...>) const
  {

    std::string s;
    for (int i = 0; fmt[i] != '\0'; i++) {
      if (fmt[i] == '%' && !('0' <= fmt[i + 1] && fmt[i + 1] <= '9')) {
        s.append("%%");
      } else {
        s.push_back(fmt[i]);
      }
    }

    boost::format f(s);
    f.exceptions(boost::io::bad_format_string_bit);
    static_cast<void>(std::initializer_list<char> {(static_cast<void>(f % std::get<Is>(args)), char{}) ...});
    return boost::str(f);
  }

public:
  template <typename ... Args>
  MessageClass(std::string&& fmt, Args&&... args) : fmt(fmt), args(std::forward<Args>(args)...)
  {
  }

  [[nodiscard]] std::string format() const
  {
    return format(std::index_sequence_for<Ts...>{});
  }
};

extern std::set<std::string> printedDeprecations;

template <typename ... Args>
void LOG(const message_group& msgGroup, Location loc, std::string docPath, std::string&& f, Args&&... args)
{
  auto formatted = MessageClass<Args...>{std::move(f), std::forward<Args>(args)...}.format();

  //check for deprecations
  if (msgGroup == message_group::Deprecated && printedDeprecations.find(formatted + loc.toRelativeString(docPath)) != printedDeprecations.end()) return;
  if (msgGroup == message_group::Deprecated) printedDeprecations.insert(formatted + loc.toRelativeString(docPath));

  Message msgObj{std::move(formatted), msgGroup, std::move(loc), std::move(docPath)};

  PRINT(msgObj);
}

template <typename ... Args>
void LOG(const message_group& msgGroup, std::string&& f, Args&&... args)
{
  LOG(msgGroup, Location::NONE, "", std::move(f), std::forward<Args>(args)...);
}

template <typename ... Args>
void LOG(std::string&& f, Args&&... args)
{
  LOG(message_group::NONE, Location::NONE, "", std::move(f), std::forward<Args>(args)...);
}
