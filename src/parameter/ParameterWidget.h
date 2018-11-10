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

#include <QTimer>

#include "parameterextractor.h"
#include "ui_ParameterWidget.h"
#include "groupwidget.h"
#include "parameterset.h"

class ParameterWidget : public QWidget, public Ui::ParameterWidget
{
	Q_OBJECT
private:
	struct groupInst {
		std::vector<std::string> parameterVector;
		bool show;
		bool inList;
	};
	std::vector<std::string> groupPos;
	typedef std::map<std::string,groupInst > group_map;
	group_map groupMap;
	QTimer autoPreviewTimer;
	DescLoD descriptionLoD; //configuration if and how much of the description is shown
	std::string jsonFile;
	bool valueChanged;
	int lastComboboxIndex = 0;

	void connectWidget();
	void updateWidget();
	void cleanScrollArea();
	void rebuildGroupMap();
	ParameterVirtualWidget* CreateParameterWidget(std::string parameterName);
	void setComboBoxPresetForSet();
	void removeChangeIndicator();

	void setFile(QString File);

	bool unreadableFileExists=false;
	entry_map_t entries;
	std::vector<std::string> ParameterPos;
	ParameterExtractor *extractor;
	ParameterSet *setMgr;

public:
	ParameterWidget(QWidget *parent = nullptr);
	~ParameterWidget();
	void readFile(QString scadFile);
	void writeFileIfNotEmpty(QString scadFile);
	void writeBackupFile(QString scadFile);
	void setParameters(const FileModule* module,bool);
	void applyParameters(FileModule *fileModule);
	bool childHasFocus();

protected slots:
	void onValueChanged();
	void onPreviewTimerElapsed();
	void onDescriptionLoDChanged();
	void onSetChanged(int idx);
	void onSetNameChanged();
	void onSetAdd();
	void onSetSaveButton();
	void onSetDelete();
	void resetParameter();
	void defaultParameter();

signals:
	void previewRequested(bool rebuildParameterUI=true);

protected:
	void applyParameterSet(std::string setName);
	void updateParameterSet(std::string setName, bool newSet=false);
	void writeParameterSets();
};

