#include "utils/printutils.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/circular_buffer.hpp>
#include <cassert>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <list>
#include <set>
#include <string>

#include "utils/exceptions.h"

namespace fs = std::filesystem;

std::set<std::string> printedDeprecations;
std::string OpenSCAD::debug("");
bool OpenSCAD::quiet = false;
bool OpenSCAD::hardwarnings = false;
int OpenSCAD::traceDepth = 12;
bool OpenSCAD::traceUsermoduleParameters = true;
bool OpenSCAD::parameterCheck = true;
bool OpenSCAD::rangeCheck = false;

namespace {

std::list<struct Message> log_messages_stack;
boost::circular_buffer<std::string> lastmessages(5);

}  // namespace

static thread_local LogClient *threadLogClient = nullptr;

ThreadLogProxy::ThreadLogProxy(LogClient *parent)
{
  // NEEDSWORK do we need to do anything when we've already got a LogClient
  // selected for this thread?
  threadLogClient = parent;
}

ThreadLogProxy::~ThreadLogProxy()
{
  // NEEDSWORK do we need to do anything when, at constructor time, we
  // already had a LogClient selected?
  // Maybe we should check to see if it's still the parent we selected,
  // and do nothing if not.
  threadLogClient = nullptr;
}

void printMessage(const Message& msgObj)
{
  if (threadLogClient) {
    threadLogClient->printMessage(msgObj);
  } else {
    // NEEDSWORK
    printf("No TLC: %s\n", msgObj.str().c_str()); fflush(stdout);
  }
}

// This should really accept the same sort of formatting that LOG() does, rather than relying on
// the macro using boost::format.
// NEEDSWORK these messages end up going to either stdio or the Console, kind of randomly.
// That's true of the old way too, though the details are a bit different.
// Note that because preview/render use ThreadLogProxy, and ~ThreadLogProxy() sets the TLC to
// nullptr, debug messages after a preview/render end up with "No TLC".
void PRINTDEBUG(const std::string& filename, const std::string& msg)
{
  // see printutils.h for usage instructions
  if (OpenSCAD::debug == "") return;
  const std::string shortfname = fs::path(filename).stem().generic_string();
  std::string lowshortfname(shortfname);
  boost::algorithm::to_lower(lowshortfname);
  std::string lowdebug(OpenSCAD::debug);
  boost::algorithm::to_lower(lowdebug);
  if (OpenSCAD::debug == "all" || lowdebug.find(lowshortfname) != std::string::npos) {
    const Message msgObj{shortfname + ": " + msg, message_group::Debug, Location::NONE, ""};
    printMessage(msgObj);
  }
}

const std::string& quoted_string(const std::string& str)
{
  static std::string buf;
  buf = str;
  boost::replace_all(buf, "\n", "\\n");
  return buf;
}

std::string two_digit_exp_format(std::string doublestr)
{
#ifdef _WIN32
  size_t exppos = doublestr.find('e');
  if (exppos != std::string::npos) {
    exppos += 2;
    if (doublestr[exppos] == '0') doublestr.erase(exppos, 1);
  }
#endif
  return doublestr;
}

std::string two_digit_exp_format(double x)
{
  return two_digit_exp_format(std::to_string(x));
}

void resetSuppressedMessages()
{
  printedDeprecations.clear();
  lastmessages.clear();
}

std::string getGroupName(const enum message_group& group)
{
  switch (group) {
  case message_group::NONE:
  case message_group::HtmlLink:       return "";
  case message_group::Warning:
  case message_group::UI_Warning:     return "WARNING";
  case message_group::Error:
  case message_group::UI_Error:       return "ERROR";
  case message_group::Font_Warning:   return "FONT-WARNING";
  case message_group::Export_Warning: return "EXPORT-WARNING";
  case message_group::Export_Error:   return "EXPORT-ERROR";
  case message_group::Parser_Error:   return "PARSER-ERROR";
  case message_group::Trace:          return "TRACE";
  case message_group::Deprecated:     return "DEPRECATED";
  case message_group::Echo:           return "ECHO";
  case message_group::Debug:          return "DEBUG";
  default:                            assert(false && "Unhandled message group name"); return "";
  }
}

std::string getGroupColor(const enum message_group& group)
{
  switch (group) {
  case message_group::Warning:
  case message_group::Deprecated:
  case message_group::UI_Warning:
  case message_group::Font_Warning: return "#ffffb0";
  case message_group::Error:
  case message_group::UI_Error:
  case message_group::Export_Error:
  case message_group::Parser_Error: return "#ffb0b0";
  case message_group::Trace:        return "#d0d0ff";
  case message_group::Debug:
  default:                          return "transparent";
  }
}

bool getGroupTextPlain(const enum message_group& group)
{
  return group == message_group::NONE || group == message_group::Echo;
}

std::string quoteVar(const std::string& varname)
{
  return '"' + varname + '"';
}

// NEEDSWORK need to ensure that we do *not* apply this to tracebacks, because
// the result is just too darned complicated, between tail call elimination, message
// deduplication, and long-trace trimming.  If we want deduplication of
// recursive calls, we should do it specially in the traceback processing
// so that it can, e.g., say "a bunch more recursive calls like that".
// (And tail call elimination should probably count the tail calls, or something.)
bool LogHost::isDuplicateMessage(const Message& msgObj)
{
  const auto msg = msgObj.str();

  // Suppress duplicate messages.
  if (msgObj.group == message_group::Warning || msgObj.group == message_group::Error ||
      msgObj.group == message_group::Trace) {
    size_t i;
    for (i = 0; i < lastmessages.size(); ++i) {
      if (lastmessages[i] != msg) break;
    }
    if (i == 5) return true;  // Suppress output after 5 equal ERROR or WARNING outputs.
    lastmessages.push_back(msg);
  }
  return false;
}

void LogHost::maybeThrow(const Message& msgObj)
{
  if (!std::current_exception()) {
    if (OpenSCAD::hardwarnings && msgObj.group == message_group::Warning) {
      throw HardWarningException(msgObj.msg);
    }
  }
}
