/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
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
#include "settings.h"
#include "printutils.h"
#include "CodeFormatter.h"

CodeFormatter * CodeFormatter::instance = NULL;

#ifdef HAVE_ASTYLE

#include <astyle.h>

void astyleErrorHandler(int errorNumber, const char* errorMessage)
{
	PRINTB("Error reformatting: error code: %d, message: %s", errorNumber % errorMessage);
}

char * astyleMemoryAlloc(unsigned long memoryNeeded)
{
	char *buffer = new(nothrow) char[memoryNeeded];
	return buffer;
}

class AStyleCodeFormatter : public CodeFormatter
{
	AStyleCodeFormatter();
	virtual ~AStyleCodeFormatter();

	bool available() const;
	std::string info() const;
	std::string format(const std::string& text) const;

	friend class CodeFormatter;
};

AStyleCodeFormatter::AStyleCodeFormatter()
{
}

AStyleCodeFormatter::~AStyleCodeFormatter()
{
}

bool AStyleCodeFormatter::available() const
{
	return true;
}

std::string AStyleCodeFormatter::info() const
{
	return std::string("Artistic Style ") + AStyleGetVersion();
}

std::string AStyleCodeFormatter::format(const std::string& text) const
{
	Settings::Settings *s = Settings::Settings::inst();

	std::string style = s->get(Settings::Settings::codeFormattingStyle).toString();
	std::string tabOrSpaces = s->get(Settings::Settings::indentStyle).toString();
	std::string tabOrSpacesOpt = (tabOrSpaces == "Tabs") ? "t" : "s";
	int indentationWidth = s->get(Settings::Settings::indentationWidth).toDouble();
	int maxCodeLineLength = s->get(Settings::Settings::maxCodeLineLength).toDouble();

	boost::format options("--style=%s,-%s%d,-xC%d,-xe,-p,-H,-U,-Y");
	options % style % tabOrSpacesOpt % indentationWidth % maxCodeLineLength;

	const char *formatted = AStyleMain(text.c_str(), options.str().c_str(), astyleErrorHandler, astyleMemoryAlloc);
	return formatted ? std::string(formatted) : std::string();
}

#endif

CodeFormatter::CodeFormatter()
{
}

CodeFormatter::~CodeFormatter()
{
}

const CodeFormatter * CodeFormatter::inst()
{
	if (instance == NULL) {
#ifdef HAVE_ASTYLE
		instance = new AStyleCodeFormatter();
#else
		instance = new CodeFormatter();
#endif
	}
	return instance;
}

bool CodeFormatter::available() const
{
	return false;
}

std::string CodeFormatter::info() const
{
	return "disabled";
}

std::string CodeFormatter::format(const std::string&) const
{
	return std::string();
}
