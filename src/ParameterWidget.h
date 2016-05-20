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

#include "qtgettext.h"
#include "ui_ParameterWidget.h"

class ParameterWidget : public QWidget, public Ui::ParameterWidget
{
	Q_OBJECT

        typedef std::map<std::string, class ParameterEntryWidget *> entry_map_t;

        QTimer autoPreviewTimer;
        entry_map_t entries;

public:
	ParameterWidget(QWidget *parent = 0);
	virtual ~ParameterWidget();

        void setParameters(const Module *module);
        void applyParameters(class FileModule *fileModule);

protected slots:
        void onValueChanged();
        void onPreviewTimerElapsed();

signals:
        void previewRequested();

protected:
        void begin();
        void addEntry(class ParameterEntryWidget *entry);
        void end();
};
