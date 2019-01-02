/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
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

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

// Dummy class for compiling Preferences.cc for Qt4
class OctoPrint
{
public:
	OctoPrint() {}
	~OctoPrint() {}

	const std::tuple<std::string, std::string, std::string> get_version() const { return {}; }
	const std::vector<std::pair<std::string, std::string>> get_slicers() const { return {}; }
	const std::vector<std::pair<std::string, std::string>> get_profiles(const QString) const { return {}; }
};

#else

class OctoPrint
{
public:
	OctoPrint();
	~OctoPrint();

	const std::string url() const;
	const std::string api_key() const;
	const std::tuple<std::string, std::string, std::string> get_version() const;
	const std::vector<std::pair<std::string, std::string>> get_slicers() const;
	const std::vector<std::pair<std::string, std::string>> get_profiles(const QString slicer) const;
	const QString upload(QFile *file, const QString fileName) const;
	void slice(const QString url, const QString slicer, const QString profile, const bool select, const bool print) const;

private:
	const QJsonDocument get_json_data(const std::string endpoint) const;

	std::string user_agent;
};

#endif