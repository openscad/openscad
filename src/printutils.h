#pragma once

#include <string>
#include <list>
#include <iostream>
#include <boost/format.hpp>

#include <libintl.h>
#include <locale.h>
inline char * _( const char * msgid ) { return gettext( msgid ); }

typedef void (OutputHandlerFunc)(const std::string &msg, void *userdata);
extern OutputHandlerFunc *outputhandler;
extern void *outputhandler_data;
namespace OpenSCAD {
	extern std::string debug;
	extern bool quiet;
}

void set_output_handler(OutputHandlerFunc *newhandler, void *userdata);

extern std::list<std::string> print_messages_stack;
void print_messages_push();
void print_messages_pop();
void printDeprecation(const std::string &str);
void resetSuppressedMessages();

#define PRINT_DEPRECATION(_fmt, _arg) do { printDeprecation(str(boost::format(_fmt) % _arg)); } while (0)

/* PRINT statements come out in same window as ECHO.
 usage: PRINTB("Var1: %s Var2: %i", var1 % var2 ); */
void PRINT(const std::string &msg);
#define PRINTB(_fmt, _arg) do { PRINT(str(boost::format(_fmt) % _arg)); } while (0)

void PRINT_NOCACHE(const std::string &msg);
#define PRINTB_NOCACHE(_fmt, _arg) do { PRINT_NOCACHE(str(boost::format(_fmt) % _arg)); } while (0)

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
