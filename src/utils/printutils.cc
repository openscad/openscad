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
std::list<std::string> print_messages_stack;
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

#ifdef  UNUSED
void LogHost::print_messages_push()
{
  print_messages_stack.emplace_back();
}

void LogHost::print_messages_pop()
{
  const std::string msg = print_messages_stack.back();
  print_messages_stack.pop_back();
  if (print_messages_stack.size() > 0 && !msg.empty()) {
    if (!print_messages_stack.back().empty()) {
      print_messages_stack.back() += "\n";
    }
    print_messages_stack.back() += msg;
  }
}
#endif  // UNUSED

void printMessage(const Message& msgObj)
{
  if (threadLogClient) {
    threadLogClient->printMessage(msgObj);
  } else {
    // NEEDSWORK
    printf("No TLC: %s\n", msgObj.str().c_str()); fflush(stdout);
  }
}

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
    const Message msgObj{shortfname + ": " + msg, message_group::NONE, Location::NONE, ""};
    // NEEDSWORK
    // PRINT_NOCACHE(msgObj);
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
  case message_group::Warning:
  case message_group::HtmlLink:
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
