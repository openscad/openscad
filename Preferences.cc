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

#include <QFontDatabase>
#include <QKeyEvent>
#include <QSettings>

Preferences *Preferences::instance = NULL;

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
	setupUi(this);

	// Setup default settings
	this->defaultmap["3dview/colorscheme"] = this->colorSchemeChooser->currentItem()->text();
	this->defaultmap["editor/fontfamily"] = this->fontChooser->currentText();
	this->defaultmap["editor/fontsize"] = this->fontSize->currentText().toUInt();

	// Toolbar
	QActionGroup *group = new QActionGroup(this);
	group->addAction(prefsAction3DView);
	group->addAction(prefsActionEditor);
	group->addAction(prefsActionAdvanced);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

	prefsAction3DView->setChecked(true);
	this->actionTriggered(this->prefsAction3DView);

	// 3D View pane
	this->colorschemes["Cornfield"][BACKGROUND_COLOR] = QColor(0xff, 0xff, 0xe5);
	this->colorschemes["Cornfield"][OPENCSG_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colorschemes["Cornfield"][OPENCSG_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colorschemes["Cornfield"][CGAL_FACE_FRONT_COLOR] = QColor(0xf9, 0xd7, 0x2c);
	this->colorschemes["Cornfield"][CGAL_FACE_BACK_COLOR] = QColor(0x9d, 0xcb, 0x51);
	this->colorschemes["Cornfield"][CGAL_FACE_2D_COLOR] = QColor(0x00, 0xbf, 0x99);
	this->colorschemes["Cornfield"][CGAL_EDGE_FRONT_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][CGAL_EDGE_BACK_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][CGAL_EDGE_2D_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][CROSSHAIR_COLOR] = QColor(0x80, 0x00, 0x00);

	this->colorschemes["Metallic"][BACKGROUND_COLOR] = QColor(0xaa, 0xaa, 0xff);
	this->colorschemes["Metallic"][OPENCSG_FACE_FRONT_COLOR] = QColor(0xdd, 0xdd, 0xff);
	this->colorschemes["Metallic"][OPENCSG_FACE_BACK_COLOR] = QColor(0xdd, 0x22, 0xdd);
	this->colorschemes["Metallic"][CGAL_FACE_FRONT_COLOR] = QColor(0xdd, 0xdd, 0xff);
	this->colorschemes["Metallic"][CGAL_FACE_BACK_COLOR] = QColor(0xdd, 0x22, 0xdd);
	this->colorschemes["Metallic"][CGAL_FACE_2D_COLOR] = QColor(0x00, 0xbf, 0x99);
	this->colorschemes["Metallic"][CGAL_EDGE_FRONT_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][CGAL_EDGE_BACK_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][CGAL_EDGE_2D_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][CROSSHAIR_COLOR] = QColor(0x80, 0x00, 0x00);

	this->colorschemes["Sunset"][BACKGROUND_COLOR] = QColor(0xaa, 0x44, 0x44);
	this->colorschemes["Sunset"][OPENCSG_FACE_FRONT_COLOR] = QColor(0xff, 0xaa, 0xaa);
	this->colorschemes["Sunset"][OPENCSG_FACE_BACK_COLOR] = QColor(0x88, 0x22, 0x33);
	this->colorschemes["Sunset"][CGAL_FACE_FRONT_COLOR] = QColor(0xff, 0xaa, 0xaa);
	this->colorschemes["Sunset"][CGAL_FACE_BACK_COLOR] = QColor(0x88, 0x22, 0x33);
	this->colorschemes["Sunset"][CGAL_FACE_2D_COLOR] = QColor(0x00, 0xbf, 0x99);
	this->colorschemes["Sunset"][CGAL_EDGE_FRONT_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][CGAL_EDGE_BACK_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][CGAL_EDGE_2D_COLOR] = QColor(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][CROSSHAIR_COLOR] = QColor(0x80, 0x00, 0x00);

	// Editor pane
	QFontDatabase db;
	foreach(int size, db.standardSizes()) {
		this->fontSize->addItem(QString::number(size));
	}

	connect(this->colorSchemeChooser, SIGNAL(itemSelectionChanged()),
					this, SLOT(colorSchemeChanged()));
	connect(this->fontChooser, SIGNAL(activated(const QString &)),
					this, SLOT(fontFamilyChanged(const QString &)));
	connect(this->fontSize, SIGNAL(editTextChanged(const QString &)),
					this, SLOT(fontSizeChanged(const QString &)));

	updateGUI();
}

Preferences::~Preferences()
{
	removeDefaultSettings();
}

void
Preferences::actionTriggered(QAction *action)
{
	if (action == this->prefsAction3DView) {
		this->stackedWidget->setCurrentWidget(this->page3DView);
	}
	else if (action == this->prefsActionEditor) {
		this->stackedWidget->setCurrentWidget(this->pageEditor);
	}
	else if (action == this->prefsActionAdvanced) {
		this->stackedWidget->setCurrentWidget(this->pageAdvanced);
	}
}

void Preferences::colorSchemeChanged()
{
	QSettings settings;
	settings.setValue("3dview/colorscheme", this->colorSchemeChooser->currentItem()->text());

	emit requestRedraw();
}

const QColor &Preferences::color(RenderColor idx)
{
	return this->colorschemes[getValue("3dview/colorscheme").toString()][idx];
}

void Preferences::fontFamilyChanged(const QString &family)
{
	QSettings settings;
	settings.setValue("editor/fontfamily", family);
	emit fontChanged(family, getValue("editor/fontsize").toUInt());
}

void Preferences::fontSizeChanged(const QString &size)
{
	uint intsize = size.toUInt();
	QSettings settings;
	settings.setValue("editor/fontsize", intsize);
	emit fontChanged(getValue("editor/fontfamily").toString(), intsize);
}

void Preferences::keyPressEvent(QKeyEvent *e)
{
#ifdef Q_WS_MAC
	if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
		close();
	} else
#endif
		if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_W ||
				e->key() == Qt::Key_Escape) {
			close();
		}
}

/*!
  Removes settings that are the same as the default settings to avoid
	overwriting future changes to default settings.
 */
void Preferences::removeDefaultSettings()
{
	QSettings settings;
	for (QSettings::SettingsMap::const_iterator iter = this->defaultmap.begin();
			 iter != this->defaultmap.end();
			 iter++) {
		if (settings.value(iter.key()) == iter.value()) {
			settings.remove(iter.key());
		}
	}
}

QVariant Preferences::getValue(const QString &key) const
{
	QSettings settings;
	return settings.value(key, this->defaultmap[key]);
}

void Preferences::updateGUI()
{
	QList<QListWidgetItem *> found = 
		this->colorSchemeChooser->findItems(getValue("3dview/colorscheme").toString(),
																				Qt::MatchExactly);
	if (!found.isEmpty()) this->colorSchemeChooser->setCurrentItem(found.first());

	QString fontfamily = getValue("editor/fontfamily").toString();
	int fidx = this->fontChooser->findText(fontfamily);
	if (fidx >= 0) {
		this->fontChooser->setCurrentIndex(fidx);
	}

	QString fontsize = getValue("editor/fontsize").toString();
	int sidx = this->fontSize->findText(fontsize);
	if (sidx >= 0) {
		this->fontSize->setCurrentIndex(sidx);
	}
	else {
		this->fontSize->setEditText(fontsize);
	}
}

void Preferences::apply() const
{
	emit fontChanged(getValue("editor/fontfamily").toString(), getValue("editor/fontsize").toUInt());
	emit requestRedraw();
}

