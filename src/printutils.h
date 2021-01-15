#pragma once

#include <string>
#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <utility>
#include <libintl.h>
#undef snprintf
#include <locale.h>
#include "AST.h"
#include <set>
inline char * _( const char * msgid ) { return gettext( msgid ); }
inline const char * _( const char * msgid, const char *msgctxt) {
	/* The separator between msgctxt and msgid in a .mo file.  */
	const char* GETTEXT_CONTEXT_GLUE = "\004";

	std::string str = msgctxt;
	str += GETTEXT_CONTEXT_GLUE;
	str += msgid;
	auto translation = dcgettext(NULL,str.c_str(), LC_MESSAGES);
	if(translation==str){
		return gettext(msgid);
	}else{
		return translation;
	}
}

enum class message_group {
	Error,Warning,UI_Warning,Font_Warning,Export_Warning,Export_Error,UI_Error,Parser_Error,Trace,Deprecated,None,Echo
};


std::string getGroupName(const enum message_group &group);
std::string getGroupColor(const enum message_group &group);
bool getGroupTextPlain(const enum message_group &group);

struct Message {
	std::string msg;
	Location loc;
	std::string docPath;
	enum message_group group;

	Message()
	: msg(""), loc(Location::NONE), docPath(""), group(message_group::None)
	{ }

	Message(const std::string& msg, const Location& loc, const std::string& docPath, const message_group& group)
	: msg(msg), loc(loc), docPath(docPath), group(group)
	{ }

	std::string str() const {
		const auto g = group == message_group::None ? "" : getGroupName(group) + ": ";
		const auto l = loc.isNone() ? "" : " " + loc.toRelativeString(docPath);
		return g + msg + l;
	}
};

typedef void (OutputHandlerFunc)(const Message &msg,void *userdata);
typedef void (OutputHandlerFunc2)(const Message &msg, void *userdata);

extern OutputHandlerFunc *outputhandler;
extern void *outputhandler_data;

namespace OpenSCAD {
	extern std::string debug;
	extern bool quiet;
	extern bool hardwarnings;
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
void PRINT(const Message &msgObj);

void PRINT_NOCACHE(const Message &msgObj);
#define PRINTB_NOCACHE(_fmt, _arg) do { } while (0)
// #define PRINTB_NOCACHE(_fmt, _arg) do { PRINT_NOCACHE(str(boost::format(_fmt) % _arg)); } while (0)

void PRINT_CONTEXT(const class Context *ctx, const class Module *mod, const class ModuleInstantiation *inst);

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

void PRINTDEBUG(const std::string &filename,const std::string &msg);
#define PRINTD(_arg) do { PRINTDEBUG(std::string(__FILE__),_arg); } while (0)
#define PRINTDB(_fmt, _arg) do { try { PRINTDEBUG(std::string(__FILE__),str(boost::format(_fmt) % _arg)); } catch(const boost::io::format_error &e) { PRINTDEBUG(std::string(__FILE__),"bad PRINTDB usage"); } } while (0)

std::string two_digit_exp_format( std::string doublestr );
std::string two_digit_exp_format( double x );
const std::string& quoted_string(const std::string& str);

// extremely simple logging, eventually replace with something like boost.log
// usage: logstream out(5); openscad_loglevel=6; out << "hi";
static int openscad_loglevel = 0;
class logstream
{
public:
	std::ostream *out;
	int loglevel;
	logstream( int level = 0 ) {
		loglevel = level;
		out = &(std::cout);
	}
	template <typename T> logstream & operator<<( T const &t ) {
		if (out && loglevel <= openscad_loglevel) {
			(*out) << t ;
			out->flush();
		}
		return *this;
	}
};

#define STR(s) static_cast<std::ostringstream&&>(std::ostringstream()<< s).str()

template <typename... Ts>
class MessageClass
{
private:
	std::string fmt;
	std::tuple<Ts...> args;
	template <std::size_t... Is>
	std::string format(const std::index_sequence<Is...>) const
	{

		std::string s;
		for(int i=0; fmt[i]!='\0'; i++) 
		{
			if(fmt[i] == '%' && !('0' <= fmt[i+1] && fmt[i+1] <= '9')) 
			{
				s.append("%%");
			} 
			else 
			{
				s.push_back(fmt[i]);
			}
		}
		
		boost::format f(s);
		f.exceptions(boost::io::bad_format_string_bit);
		static_cast<void>(std::initializer_list<char> {(static_cast<void>(f % std::get<Is>(args)), char{}) ...});
		return boost::str(f);
	}

public:
	template <typename... Args>
	MessageClass(std::string&& fmt, Args&&... args) : fmt(std::forward<std::string>(fmt)), args(std::forward<Args>(args)...)
	{
	}

	std::string format() const
	{
	return format(std::index_sequence_for<Ts...>{});
	}
};

extern std::set<std::string> printedDeprecations;

template <typename F, typename... Args>
void LOG(const message_group &msg_grp,const Location &loc,const std::string &docPath,F&& f, Args&&... args)
{	
	const auto msg = MessageClass<Args...>(std::forward<F>(f), std::forward<Args>(args)...);
	const auto formatted = msg.format();

	//check for deprecations
	if (msg_grp == message_group::Deprecated && printedDeprecations.find(formatted+loc.toRelativeString(docPath)) != printedDeprecations.end()) return;
	if(msg_grp == message_group::Deprecated) printedDeprecations.insert(formatted+loc.toRelativeString(docPath));

	Message msgObj = {formatted,loc,docPath,msg_grp};

	PRINT(msgObj);
}
