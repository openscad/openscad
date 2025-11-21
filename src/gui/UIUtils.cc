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

#include "gui/UIUtils.h"

#include <filesystem>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <exception>
#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QFileInfoList>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonDocument>

#include "version.h"
#include "platform/PlatformUtils.h"
#include "gui/QSettingsCached.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

QString fileOpenFilter(const QString& pattern, QStringList extensions)
{
  if (extensions.isEmpty()) {
    extensions << "scad" << "csg";
#ifdef ENABLE_PYTHON
    extensions << "py";
#endif
  }
  extensions.replaceInStrings(QRegularExpression("^"), "*.");
  return pattern.arg(extensions.join(" "));
}

QList<UIUtils::ExampleCategory> _exampleCategories;
QMap<QString, QList<UIUtils::ExampleEntry>> _examples;

bool hasCategory(const QString& name)
{
  for (const auto& category : _exampleCategories) {
    if (category.name == name) {
      return true;
    }
  }
  return false;
}

void readExamplesDir(const QJsonObject& obj, const fs::path& dir)
{
  QString name = obj["name"].toString(QString::fromStdString(dir.filename().generic_string()));

  if (!hasCategory(name)) {
    _exampleCategories.append(
      UIUtils::ExampleCategory{.sort = obj["sort"].toInt(UIUtils::ExampleCategory::DEFAULT_SORT),
                               .name = name,
                               .tooltip = obj["tooltip"].toString()});
  }

  auto& examples = _examples[name];
  for (const auto& entry : fs::directory_iterator{dir}) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto& path = entry.path();
    if (path.extension() != ".scad") {
      continue;
    }
    examples.append(
      UIUtils::ExampleEntry{.name = QString::fromStdString(path.filename().generic_string()),
                            .fileInfo = QFileInfo(QString::fromStdString(path.generic_string()))});
  }
  std::sort(examples.begin(), examples.end(),
            [](const UIUtils::ExampleEntry& e1, const UIUtils::ExampleEntry& e2) -> bool {
              return e1.name < e2.name;
            });
}

void enumerateExamples(const fs::path& dir)
{
  if (!fs::is_directory(dir)) {
    return;
  }
  for (const auto& entry : fs::directory_iterator{dir}) {
    if (!entry.is_directory()) {
      continue;
    }
    auto fileInfo =
      QFileInfo{QDir{QString::fromStdString(entry.path().generic_string())}, "example-dir.json"};
    QJsonObject obj;
    QFile file(fileInfo.filePath());
    if (file.open(QIODevice::ReadOnly)) {
      obj = QJsonDocument::fromJson(file.readAll()).object();
    }
    readExamplesDir(obj, entry.path());
  }
  std::sort(_exampleCategories.begin(), _exampleCategories.end(),
            [](const UIUtils::ExampleCategory& c1, const UIUtils::ExampleCategory& c2) -> bool {
              return c2.sort > c1.sort;
            });
}

const QList<UIUtils::ExampleCategory>& readExamples()
{
  if (_exampleCategories.empty()) {
    enumerateExamples(fs::path{PlatformUtils::resourceBasePath()} / "examples");
    enumerateExamples(PlatformUtils::userExamplesPath());
  }
  return _exampleCategories;
}

}  // namespace

QFileInfo UIUtils::openFile(QWidget *parent, QStringList extensions)
{
  QSettingsCached settings;
  const auto last_dirname = settings.value("lastOpenDirName").toString();
  const auto filter = fileOpenFilter("OpenSCAD Designs (%1)", std::move(extensions));
  const auto filename = QFileDialog::getOpenFileName(parent, "Open File", last_dirname, filter);

  if (filename.isEmpty()) {
    return {};
  }

  QFileInfo fileInfo(filename);
  settings.setValue("lastOpenDirName", fileInfo.dir().path());
  return fileInfo;
}

QFileInfoList UIUtils::openFiles(QWidget *parent, QStringList extensions)
{
  QSettingsCached settings;
  const auto last_dirname = settings.value("lastOpenDirName").toString();
  const auto filter = fileOpenFilter("OpenSCAD Designs (%1)", std::move(extensions));
  const auto filenames = QFileDialog::getOpenFileNames(parent, "Open File", last_dirname, filter);

  QFileInfoList fileInfoList;
  for (const QString& filename : filenames) {
    if (filename.isEmpty()) {
      continue;
    }
    fileInfoList.append(QFileInfo(filename));
  }

  if (!fileInfoList.isEmpty()) {
    // last_dir is set to directory of last chosen valid file
    settings.setValue("lastOpenDirName", fileInfoList.back().dir().path());
  }

  return fileInfoList;
}

QStringList UIUtils::recentFiles()
{
  QSettingsCached settings;  // set up project and program properly in main.cpp
  QStringList files = settings.value("recentFileList").toStringList();

  // Remove any duplicate or empty entries from the list
  files.removeDuplicates();
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

const QList<UIUtils::ExampleCategory>& UIUtils::exampleCategories() { return readExamples(); }

QFileInfoList UIUtils::exampleFiles(const QString& category)
{
  QFileInfoList examples;
  if (!_examples.contains(category)) {
    return examples;
  }
  for (const auto& e : _examples[category]) {
    examples << e.fileInfo;
  }
  return examples;
}

void UIUtils::openURL(const QString& url) { QDesktopServices::openUrl(QUrl(url)); }

void UIUtils::openHomepageURL() { QDesktopServices::openUrl(QUrl("https://www.openscad.org/")); }

static void openVersionedURL(const QString& url)
{
  QDesktopServices::openUrl(QUrl(url.arg(openscad_shortversionnumber.data())));
}

void UIUtils::openUserManualURL()
{
  openVersionedURL("https://www.openscad.org/documentation.html?version=%1");
}

fs::path UIUtils::returnOfflineUserManualPath()
{
  fs::path resPath = PlatformUtils::resourcePath("resources");
  fs::path fullPath =
    resPath / "docs" / "OpenSCADUserDocs" / "openscad_docs" / "OpenSCAD_User_Manual.html";
  return fullPath;
}

bool UIUtils::hasOfflineUserManual()
{
  fs::path fullPath = returnOfflineUserManualPath();
  if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
    return true;
  }
  return false;
}

void UIUtils::openOfflineUserManual()
{
  fs::path fullPath = returnOfflineUserManualPath();
  if (UIUtils::hasOfflineUserManual()) {
    QString docPath = QString::fromStdString(fullPath.string());
    QDesktopServices::openUrl(QUrl(docPath));
  }
}

void UIUtils::openCheatSheetURL()
{
#ifdef OPENSCAD_SNAPSHOT
  openVersionedURL("https://www.openscad.org/cheatsheet/snapshot.html?version=%1");
#else
  openVersionedURL("https://www.openscad.org/cheatsheet/index.html?version=%1");
#endif
}

fs::path UIUtils::returnOfflineCheatSheetPath()
{
  fs::path resPath = PlatformUtils::resourcePath("resources");
  fs::path fullPath = resPath / "docs" / "OpenSCADUserDocs" / "openscad_docs" / "CheatSheet.html";
  return fullPath;
}

bool UIUtils::hasOfflineCheatSheet()
{
  fs::path fullPath = returnOfflineCheatSheetPath();
  if (fs::exists(fullPath) && fs::is_regular_file(fullPath)) {
    return true;
  }
  return false;
}

void UIUtils::openOfflineCheatSheet()
{
  fs::path fullPath = returnOfflineCheatSheetPath();
  if (UIUtils::hasOfflineCheatSheet()) {
    QString docPath = QString::fromStdString(fullPath.string());
    QDesktopServices::openUrl(QUrl(docPath));
  }
}

QString UIUtils::getBackgroundColorStyleSheet(const QColor& color)
{
  return QString("background-color:%1;").arg(color.toRgb().name());
}

QString UIUtils::blendForBackgroundColorStyleSheet(const QColor& input, const QColor& blend,
                                                   float transparency)
{
  const auto result =
    QColor(255.0 * (transparency * blend.redF() + (1 - transparency) * input.redF()),
           255.0 * (transparency * blend.greenF() + (1 - transparency) * input.greenF()),
           255.0 * (transparency * blend.blueF() + (1 - transparency) * input.blueF()));
  return getBackgroundColorStyleSheet(result);
}
