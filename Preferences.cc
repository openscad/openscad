/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "Preferences.h"

Preferences *Preferences::instance = NULL;

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
	setupUi(this);

	this->colormap[BACKGROUND_COLOR] = QColor(0xff, 0xff, 0xe5);
	this->colormap[OPENCSG_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colormap[OPENCSG_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colormap[CGAL_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colormap[CGAL_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colormap[CGAL_FACE_2D_COLOR] = QColor(0x00, 0xbf, 0x99);
	this->colormap[CGAL_EDGE_FRONT_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colormap[CGAL_EDGE_BACK_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colormap[CGAL_EDGE_2D_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colormap[CROSSHAIR_COLOR] = QColor(0x80, 0x00, 0x00);

	QActionGroup *group = new QActionGroup(this);
	group->addAction(prefsAction3DView);
	group->addAction(prefsActionAdvanced);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

	prefsAction3DView->setChecked(true);
	this->actionTriggered(this->prefsAction3DView);
}

Preferences::~Preferences()
{
}

void
Preferences::actionTriggered(QAction *action)
{
	if (action == this->prefsAction3DView) {
		this->stackedWidget->setCurrentWidget(this->page3DView);
	}
	else if (action == this->prefsActionAdvanced) {
		this->stackedWidget->setCurrentWidget(this->pageAdvanced);
	}
}
