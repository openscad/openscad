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

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>

#include "qtgettext.h"
#include "UIUtils.h"
#include "PlatformUtils.h"
#include "openscad.h"
#include "QSettingsCached.h"


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

QFileInfo UIUtils::openFile(QWidget *parent)
{
	QSettingsCached settings;
	QString last_dirname = settings.value("lastOpenDirName").toString();
	QString new_filename = QFileDialog::getOpenFileName(parent, "Open File",
																											last_dirname, "OpenSCAD Designs (*.scad *.csg)");

	if (new_filename.isEmpty()) {
		return QFileInfo();
	}

	QFileInfo fileInfo(new_filename);
	QDir last_dir = fileInfo.dir();
	last_dirname = last_dir.path();
	settings.setValue("lastOpenDirName", last_dirname);
	return fileInfo;
}

QStringList UIUtils::recentFiles()
{
	QSettingsCached settings;   // set up project and program properly in main.cpp
	QStringList files = settings.value("recentFileList").toStringList();

	// Remove any duplicate or empty entries from the list
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
	files.removeDuplicates();
#endif
	files.removeAll(QString());
	// Now remove any entries which do not exist
	for (int i = files.size() - 1; i >= 0; --i) {
		QFileInfo fileInfo(files[i]);
		if (!QFile(fileInfo.absoluteFilePath()).exists()) files.removeAt(i);
	}

	while (files.size() > UIUtils::maxRecentFiles) {
		files.removeAt(files.size() - 1);
	}

	settings.setValue("recentFileList", files);
	return files;
}

using namespace boost::property_tree;

static ptree *examples_tree = nullptr;
static ptree *examplesTree()
{
	if (!examples_tree) {
		std::string path = (PlatformUtils::resourcePath("examples") / "examples.json").string();
		try {
			examples_tree = new ptree;
			read_json(path, *examples_tree);
		}
		catch (const std::exception &e) {
			PRINTB("Error reading examples.json: %s", e.what());
			delete examples_tree;
			examples_tree = nullptr;
		}
	}
	return examples_tree;
}

QStringList UIUtils::exampleCategories()
{
	// categories in File menu item - Examples
	QStringList categories;
	ptree *pt = examplesTree();
	if (pt) {
		for (const auto &v : *pt) {
			// v.first is the name of the child.
			// v.second is the child tree.
			categories << QString::fromStdString(v.first);
		}
	}

	return categories;
}

QFileInfoList UIUtils::exampleFiles(const QString &category)
{
	QFileInfoList examples;
	ptree *pt = examplesTree();
	if (pt) {
		fs::path examplesPath = PlatformUtils::resourcePath("examples") / category.toStdString();
		for (const auto &v : pt->get_child(category.toStdString())) {
			examples << QFileInfo(QString::fromStdString((examplesPath / v.second.data()).string()));
		}
	}
	return examples;
}

void UIUtils::openHomepageURL()
{
	QDesktopServices::openUrl(QUrl("http://openscad.org/"));
}

static void openVersionedURL(QString url)
{
	QDesktopServices::openUrl(QUrl(url.arg(openscad_shortversionnumber.c_str())));
}

void UIUtils::openUserManualURL()
{
	openVersionedURL("http://www.openscad.org/documentation.html?version=%1");
}

void UIUtils::openCheatSheetURL()
{
	openVersionedURL("http://www.openscad.org/cheatsheet/index.html?version=%1");
}
