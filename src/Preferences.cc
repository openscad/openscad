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

#include "Preferences.h"

#include <QMessageBox>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QSettings>
#include <QStatusBar>
#include "PolySetCache.h"
#include "AutoUpdater.h"
#ifdef ENABLE_CGAL
#include "CGALCache.h"
#endif

Preferences *Preferences::instance = NULL;

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
	setupUi(this);

	// Editor pane
	// Setup default font (Try to use a nice monospace font)
	QString fontfamily;
#ifdef Q_WS_X11
	fontfamily = "Mono";
#elif defined (Q_WS_WIN)
	fontfamily = "Console";
#elif defined (Q_WS_MAC)
	fontfamily = "Monaco";
#endif
	QFont font;
	font.setStyleHint(QFont::TypeWriter);
	font.setFamily(fontfamily); // this runs Qt's font matching algorithm
	QString found_family(QFontInfo(font).family());
	this->defaultmap["editor/fontfamily"] = found_family;
 	this->defaultmap["editor/fontsize"] = 12;

	uint savedsize = getValue("editor/fontsize").toUInt();
	QFontDatabase db;
	foreach(uint size, db.standardSizes()) {
		this->fontSize->addItem(QString::number(size));
		if (size == savedsize) {
			this->fontSize->setCurrentIndex(this->fontSize->count()-1);
		}
	}

	connect(this->fontSize, SIGNAL(currentIndexChanged(const QString&)),
					this, SLOT(on_fontSize_editTextChanged(const QString &)));

	// reset GUI fontsize if fontSize->addItem emitted signals that changed it.
	this->fontSize->setEditText( QString("%1").arg( savedsize ) );

	// Setup default settings
	this->defaultmap["3dview/colorscheme"] = this->colorSchemeChooser->currentItem()->text();
	this->defaultmap["advanced/opencsg_show_warning"] = true;
	this->defaultmap["advanced/enable_opencsg_opengl1x"] = true;
	this->defaultmap["advanced/polysetCacheSize"] = uint(PolySetCache::instance()->maxSize());
#ifdef ENABLE_CGAL
	this->defaultmap["advanced/cgalCacheSize"] = uint(CGALCache::instance()->maxSize());
#endif
	this->defaultmap["advanced/openCSGLimit"] = RenderSettings::inst()->openCSGTermLimit;
	this->defaultmap["advanced/forceGoldfeather"] = false;


	// Toolbar
	QActionGroup *group = new QActionGroup(this);
	group->addAction(prefsAction3DView);
	group->addAction(prefsActionEditor);
	group->addAction(prefsActionUpdate);
	group->addAction(prefsActionAdvanced);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

	prefsAction3DView->setChecked(true);
	this->actionTriggered(this->prefsAction3DView);

	// 3D View pane
	this->colorschemes["Cornfield"][RenderSettings::BACKGROUND_COLOR] = Color4f(0xff, 0xff, 0xe5);
	this->colorschemes["Cornfield"][RenderSettings::OPENCSG_FACE_FRONT_COLOR] = Color4f(0xf9, 0xd7, 0x2c);
	this->colorschemes["Cornfield"][RenderSettings::OPENCSG_FACE_BACK_COLOR] = Color4f(0x9d, 0xcb, 0x51);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_FACE_FRONT_COLOR] = Color4f(0xf9, 0xd7, 0x2c);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_FACE_BACK_COLOR] = Color4f(0x9d, 0xcb, 0x51);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_FACE_2D_COLOR] = Color4f(0x00, 0xbf, 0x99);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_EDGE_FRONT_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_EDGE_BACK_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][RenderSettings::CGAL_EDGE_2D_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Cornfield"][RenderSettings::CROSSHAIR_COLOR] = Color4f(0x80, 0x00, 0x00);

	this->colorschemes["Metallic"][RenderSettings::BACKGROUND_COLOR] = Color4f(0xaa, 0xaa, 0xff);
	this->colorschemes["Metallic"][RenderSettings::OPENCSG_FACE_FRONT_COLOR] = Color4f(0xdd, 0xdd, 0xff);
	this->colorschemes["Metallic"][RenderSettings::OPENCSG_FACE_BACK_COLOR] = Color4f(0xdd, 0x22, 0xdd);
	this->colorschemes["Metallic"][RenderSettings::CGAL_FACE_FRONT_COLOR] = Color4f(0xdd, 0xdd, 0xff);
	this->colorschemes["Metallic"][RenderSettings::CGAL_FACE_BACK_COLOR] = Color4f(0xdd, 0x22, 0xdd);
	this->colorschemes["Metallic"][RenderSettings::CGAL_FACE_2D_COLOR] = Color4f(0x00, 0xbf, 0x99);
	this->colorschemes["Metallic"][RenderSettings::CGAL_EDGE_FRONT_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][RenderSettings::CGAL_EDGE_BACK_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][RenderSettings::CGAL_EDGE_2D_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Metallic"][RenderSettings::CROSSHAIR_COLOR] = Color4f(0x80, 0x00, 0x00);

	this->colorschemes["Sunset"][RenderSettings::BACKGROUND_COLOR] = Color4f(0xaa, 0x44, 0x44);
	this->colorschemes["Sunset"][RenderSettings::OPENCSG_FACE_FRONT_COLOR] = Color4f(0xff, 0xaa, 0xaa);
	this->colorschemes["Sunset"][RenderSettings::OPENCSG_FACE_BACK_COLOR] = Color4f(0x88, 0x22, 0x33);
	this->colorschemes["Sunset"][RenderSettings::CGAL_FACE_FRONT_COLOR] = Color4f(0xff, 0xaa, 0xaa);
	this->colorschemes["Sunset"][RenderSettings::CGAL_FACE_BACK_COLOR] = Color4f(0x88, 0x22, 0x33);
	this->colorschemes["Sunset"][RenderSettings::CGAL_FACE_2D_COLOR] = Color4f(0x00, 0xbf, 0x99);
	this->colorschemes["Sunset"][RenderSettings::CGAL_EDGE_FRONT_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][RenderSettings::CGAL_EDGE_BACK_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][RenderSettings::CGAL_EDGE_2D_COLOR] = Color4f(0xff, 0x00, 0x00);
	this->colorschemes["Sunset"][RenderSettings::CROSSHAIR_COLOR] = Color4f(0x80, 0x00, 0x00);

  // Advanced pane	
	QValidator *validator = new QIntValidator(this);
#ifdef ENABLE_CGAL
	this->cgalCacheSizeEdit->setValidator(validator);
#endif
	this->polysetCacheSizeEdit->setValidator(validator);
	this->opencsgLimitEdit->setValidator(validator);

	updateGUI();

	RenderSettings::inst()->setColors(this->colorschemes[getValue("3dview/colorscheme").toString()]);
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
	else if (action == this->prefsActionUpdate) {
		this->stackedWidget->setCurrentWidget(this->pageUpdate);
	}
	else if (action == this->prefsActionAdvanced) {
		this->stackedWidget->setCurrentWidget(this->pageAdvanced);
	}
}

void Preferences::on_colorSchemeChooser_itemSelectionChanged()
{
	QString scheme = this->colorSchemeChooser->currentItem()->text();
	QSettings settings;
	settings.setValue("3dview/colorscheme", scheme);

	RenderSettings::inst()->setColors(this->colorschemes[scheme]);

	emit requestRedraw();
}

void Preferences::on_fontChooser_activated(const QString &family)
{
	QSettings settings;
	settings.setValue("editor/fontfamily", family);
	emit fontChanged(family, getValue("editor/fontsize").toUInt());
}

void Preferences::on_fontSize_editTextChanged(const QString &size)
{
	uint intsize = size.toUInt();
	QSettings settings;
	settings.setValue("editor/fontsize", intsize);
	emit fontChanged(getValue("editor/fontfamily").toString(), intsize);
}

void unimplemented_msg()
{
  QMessageBox mbox;
	mbox.setText("Sorry, this feature is not implemented on your Operating System");
	mbox.exec();
}

void Preferences::on_updateCheckBox_toggled(bool on)
{
	if (AutoUpdater *updater =AutoUpdater::updater()) {
		updater->setAutomaticallyChecksForUpdates(on);
	} else {
		unimplemented_msg();
	}
}

void Preferences::on_snapshotCheckBox_toggled(bool on)
{
	if (AutoUpdater *updater =AutoUpdater::updater()) {
		updater->setEnableSnapshots(on);
	} else {
		unimplemented_msg();
	}
}

void Preferences::on_checkNowButton_clicked()
{
	if (AutoUpdater *updater =AutoUpdater::updater()) {
		updater->checkForUpdates();
	} else {
		unimplemented_msg();
	}
}

void
Preferences::on_openCSGWarningBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/opencsg_show_warning",state);
}

void
Preferences::on_enableOpenCSGBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/enable_opencsg_opengl1x", state);
}

void Preferences::on_cgalCacheSizeEdit_textChanged(const QString &text)
{
	QSettings settings;
	settings.setValue("advanced/cgalCacheSize", text);
#ifdef ENABLE_CGAL
	CGALCache::instance()->setMaxSize(text.toULong());
#endif
}

void Preferences::on_polysetCacheSizeEdit_textChanged(const QString &text)
{
	QSettings settings;
	settings.setValue("advanced/polysetCacheSize", text);
	PolySetCache::instance()->setMaxSize(text.toULong());
}

void Preferences::on_opencsgLimitEdit_textChanged(const QString &text)
{
	QSettings settings;
	settings.setValue("advanced/openCSGLimit", text);
	// FIXME: Set this globally?
}

void Preferences::on_forceGoldfeatherBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/forceGoldfeather", state);
	emit openCSGSettingsChanged();
}

void Preferences::keyPressEvent(QKeyEvent *e)
{
#ifdef Q_WS_MAC
	if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
		close();
	} else
#endif
		if ((e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_W) ||
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
	assert(settings.contains(key) || this->defaultmap.contains(key));
	return settings.value(key, this->defaultmap[key]);
}

void Preferences::updateGUI()
{
	QSettings settings;
	QList<QListWidgetItem *> found = 
		this->colorSchemeChooser->findItems(getValue("3dview/colorscheme").toString(),
																				Qt::MatchExactly);
	if (!found.isEmpty()) this->colorSchemeChooser->setCurrentItem(found.first());

	QString fontfamily = getValue("editor/fontfamily").toString();
	int fidx = this->fontChooser->findText(fontfamily,Qt::MatchContains);
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

	if (AutoUpdater *updater = AutoUpdater::updater()) {
		this->updateCheckBox->setChecked(updater->automaticallyChecksForUpdates());
		this->snapshotCheckBox->setChecked(updater->enableSnapshots());
		this->lastCheckedLabel->setText(updater->lastUpdateCheckDate());
	}

	this->openCSGWarningBox->setChecked(getValue("advanced/opencsg_show_warning").toBool());
	this->enableOpenCSGBox->setChecked(getValue("advanced/enable_opencsg_opengl1x").toBool());
	this->cgalCacheSizeEdit->setText(getValue("advanced/cgalCacheSize").toString());
	this->polysetCacheSizeEdit->setText(getValue("advanced/polysetCacheSize").toString());
	this->opencsgLimitEdit->setText(getValue("advanced/openCSGLimit").toString());
	this->forceGoldfeatherBox->setChecked(getValue("advanced/forceGoldfeather").toBool());
}

void Preferences::apply() const
{
	emit fontChanged(getValue("editor/fontfamily").toString(), getValue("editor/fontsize").toUInt());
	emit requestRedraw();
	emit openCSGSettingsChanged();
}
