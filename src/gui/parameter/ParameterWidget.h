/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2014 Clifford Wolf <clifford@clifford.at> and
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

#include <QString>
#include <QWidget>
#include <QTimer>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "gui/qtgettext.h" // IWYU pragma: keep
#include "ui_ParameterWidget.h"
#include "core/customizer/ParameterObject.h"
#include "core/customizer/ParameterSet.h"
#include "gui/parameter/ParameterVirtualWidget.h"

class ParameterWidget : public QWidget, public Ui::ParameterWidget
{
  Q_OBJECT
private:
  ParameterSets sets;
  std::string source;
  ParameterObjects parameters;
  std::map<ParameterObject *, std::vector<ParameterVirtualWidget *>> widgets;

  QString invalidJsonFile; // set if a json file was read that could not be parsed
  QTimer autoPreviewTimer;
  bool modified = false;

public:
  ParameterWidget(QWidget *parent = nullptr);
  void readFile(const QString& scadFile);
  void saveFile(const QString& scadFile);
  void saveBackupFile(const QString& scadFile);
  void setParameters(const SourceFile *sourceFile, const std::string& source);
  void applyParameters(SourceFile *sourceFile);
  bool childHasFocus();
  bool isModified() const { return modified; }

public slots:
  void setModified(bool modified = true);
  void setFontFamilySize(const QString &fontfamily, uint fontsize);

protected slots:
  void autoPreview(bool immediate = false);
  void emitParametersChanged();
  void onSetChanged(int index);
  void onSetNameChanged();
  void onSetAdd();
  void onSetDelete();
  void parameterModified(bool immediate);
  void loadSet(size_t index);
  void createSet(const QString& name);
  void updateSetEditability();
  void rebuildWidgets();

signals:
  // emitted when the effective values of the parameters have changed,
  // and the model view can be updated
  void parametersChanged();
  // emitted when the sets that would be saved to the json file have changed,
  // and the parameter sets should be saved before closing
  void modificationChanged();

protected:
  struct ParameterGroup
  {
    QString name;
    std::vector<ParameterObject *> parameters;
  };
  std::vector<ParameterGroup> getParameterGroups();
  ParameterVirtualWidget *createParameterWidget(ParameterObject *parameter, DescriptionStyle descriptionStyle);
  QString getJsonFile(const QString& scadFile);
  void cleanSets();
};
