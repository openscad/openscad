#include "printutils.h"
#include <sstream>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include "exceptions.h"


namespace fs = boost::filesystem;

std::set<std::string> printedDeprecations;
std::list<std::string> print_messages_stack;
std::list<struct Message> log_messages_stack;
OutputHandlerFunc *outputhandler = nullptr;
OutputHandlerFunc2 *outputhandler2 = nullptr;
void *outputhandler_data = nullptr;
std::string OpenSCAD::debug("");
bool OpenSCAD::quiet = false;
bool OpenSCAD::hardwarnings = false;
bool OpenSCAD::parameterCheck = true;
bool OpenSCAD::rangeCheck = false;

boost::circular_buffer<std::string> lastmessages(5);
boost::circular_buffer<struct Message> lastlogmessages(5);

int count=0;

namespace {
	bool no_throw;
	bool deferred;
}

void set_output_handler(OutputHandlerFunc *newhandler, OutputHandlerFunc2 *newhandler2, void *userdata)
{
	outputhandler = newhandler;
	outputhandler2 = newhandler2;
	outputhandler_data = userdata;
}

void no_exceptions_for_warnings()
{
	no_throw = true;
	deferred = false;
}

bool would_have_thrown()
{
    const auto would_throw = deferred;
    no_throw = false;
    deferred = false;
    return would_throw;
}

void print_messages_push()
{
	print_messages_stack.push_back(std::string());
}

void print_messages_pop()
{
	std::string msg = print_messages_stack.back();
	print_messages_stack.pop_back();
	if (print_messages_stack.size() > 0 && !msg.empty()) {
		if (!print_messages_stack.back().empty()) {
			print_messages_stack.back() += "\n";
		}
		print_messages_stack.back() += msg;
	}
}

void PRINT(const Message& msgObj)
{
	if (msgObj.msg.empty() && msgObj.group != message_group::Echo) return;

	if (print_messages_stack.size() > 0) {
		if (!print_messages_stack.back().empty()) {
			print_messages_stack.back() += "\n";
		}
		print_messages_stack.back() += msgObj.str();
	}

	PRINT_NOCACHE(msgObj);

	//to error log
	if (outputhandler2 && 
	    !(msgObj.group==message_group::None || msgObj.group==message_group::Echo || msgObj.group==message_group::Trace)) {
		
		outputhandler2(msgObj, outputhandler_data);
	}
}

void PRINT_NOCACHE(const Message& msgObj)
{
	if (msgObj.msg.empty() && msgObj.group != message_group::Echo) return;

	const auto msg = msgObj.str();

	if (msgObj.group == message_group::Warning || msgObj.group == message_group::Error || msgObj.group == message_group::Trace) {
		size_t i;
		for (i = 0; i < lastmessages.size(); ++i) {
			if (lastmessages[i] != msg) break;
		}
		if (i == 5) return; // Suppress output after 5 equal ERROR or WARNING outputs.
		lastmessages.push_back(msg);
	}
	if(!deferred)
		if (!OpenSCAD::quiet || msgObj.group == message_group::Error) {
			if (!outputhandler) {
				std::cerr << msg << "\n";
			} else {
				outputhandler(msgObj,outputhandler_data);
			}
		}
	if(!std::current_exception()) {
		if ((OpenSCAD::hardwarnings && msgObj.group == message_group::Warning) || (no_throw && msgObj.group == message_group::Error)) {
			if(no_throw)
				deferred = true;
			else
				throw HardWarningException(msgObj.msg);
		}
	}
}

void PRINTDEBUG(const std::string &filename, const std::string &msg)
{
	// see printutils.h for usage instructions
	if (OpenSCAD::debug=="") return;
	std::string shortfname = fs::path(filename).stem().generic_string();
	std::string lowshortfname(shortfname);
	boost::algorithm::to_lower(lowshortfname);
	std::string lowdebug(OpenSCAD::debug);
	boost::algorithm::to_lower(lowdebug);
	if (OpenSCAD::debug=="all" ||
			lowdebug.find(lowshortfname) != std::string::npos) {
			Message msgObj = {shortfname+": "+ msg,Location::NONE,"",message_group::None,};
		PRINT_NOCACHE(msgObj);
	}
}

const std::string& quoted_string(const std::string& str)
{
	static std::string buf;
	buf = str;
	boost::replace_all(buf, "\n", "\\n");
	return buf;
}

std::string two_digit_exp_format( std::string doublestr )
{
#ifdef _WIN32
	size_t exppos = doublestr.find('e');
	if ( exppos != std::string::npos) {
		exppos += 2;
		if ( doublestr[exppos] == '0' ) doublestr.erase(exppos,1);
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
	switch (group)
	{
	case message_group::None:
	case message_group::Warning:
	case message_group::UI_Warning:
		return "WARNING";
	case message_group::Error:
	case message_group::UI_Error:
		return "ERROR";
	case message_group::Font_Warning:
		return "FONT-WARNING";
	case message_group::Export_Warning:
		return "EXPORT-WARNING";
	case message_group::Export_Error:
		return "EXPORT-ERROR";
	case message_group::Parser_Error:
		return "PARSER-ERROR";
	case message_group::Trace:
		return "TRACE";
	case message_group::Deprecated:
		return "DEPRECATED";
	case message_group::Echo:
		return "ECHO";
	default:
		assert(false && "Unhandled message group name");
		return "";
	}
}

std::string getGroupColor(const enum message_group& group)
{
	switch (group) {
	case message_group::Warning:
	case message_group::Deprecated:
	case message_group::UI_Warning:
	case message_group::Font_Warning:
		return "#ffffb0";
	case message_group::Error:
	case message_group::UI_Error:
	case message_group::Export_Error:
	case message_group::Parser_Error:
		return "#ffb0b0";
	case message_group::Trace:
		return "#d0d0ff";
	default:
		return "#ffffff";
	}
}

bool getGroupTextPlain(const enum message_group& group)
{
	return group == message_group::None || group == message_group::Echo;
}