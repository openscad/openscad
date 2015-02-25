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
#include <QSettings>
#include <QFileInfo>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>

#include "qtgettext.h"
#include "UIUtils.h"
#include "PlatformUtils.h"
#include "openscad.h"

QFileInfo UIUtils::openFile(QWidget *parent)
{
    QSettings settings;
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
    QSettings settings; // set up project and program properly in main.cpp
    QStringList files = settings.value("recentFileList").toStringList();

    // Remove any duplicate or empty entries from the list
#if (QT_VERSION >= QT_VERSION_CHECK(4, 5, 0))
    files.removeDuplicates();
#endif
    files.removeAll(QString());
    // Now remove any entries which do not exist
    for (int i = files.size() - 1; i >= 0; --i) {
	QFileInfo fileInfo(files[i]);
	if (!QFile(fileInfo.absoluteFilePath()).exists())
	    files.removeAt(i);
    }
    
    while (files.size() > UIUtils::maxRecentFiles) {
	files.removeAt(files.size() - 1);
    }

    settings.setValue("recentFileList", files);
    return files;
}

QStringList UIUtils::exampleCategories()
{
    QStringList categories;
    //categories in File menu item - Examples
    categories << N_("Basics") << N_("Shapes") << N_("Extrusion") << N_("Advanced");
    
    return categories;
}

QFileInfoList UIUtils::exampleFiles(const QString &category)
{
    QDir dir(QString::fromStdString(PlatformUtils::resourcePath("examples").string()));
    if (!dir.cd(category)) {
	return QFileInfoList();
    }

    QFileInfoList examples = dir.entryInfoList(QStringList("*.scad"), QDir::Files | QDir::Readable, QDir::Name);
    return examples;
}

void UIUtils::openHomepageURL()
{
    QDesktopServices::openUrl(QUrl("http://openscad.org/"));
}

static void openVersionedURL(QString url)
{
    QDesktopServices::openUrl(QUrl(url.arg(versionnumber.c_str())));
}

void UIUtils::openUserManualURL()
{
    openVersionedURL("http://www.openscad.org/documentation.html?version=%1");
}

void UIUtils::openCheatSheetURL()
{
    openVersionedURL("http://www.openscad.org/cheatsheet/index.html?version=%1");
}
