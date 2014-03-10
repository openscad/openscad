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
#include "feature.h"
#ifdef ENABLE_CGAL
#include "CGALCache.h"
#endif

Preferences *Preferences::instance = NULL;

const char * Preferences::featurePropertyName = "FeatureProperty";
Q_DECLARE_METATYPE(Feature *);

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
	setupUi(this);

	// Editor pane
	// Setup default font (Try to use a nice monospace font)
	QString fontfamily;
#ifdef Q_OS_X11
	fontfamily = "Mono";
#elif defined (Q_OS_WIN)
	fontfamily = "Console";
#elif defined (Q_OS_MAC)
	fontfamily = "Monaco";
#endif
	QFont font;
	font.setStyleHint(QFont::TypeWriter);
	font.setFamily(fontfamily); // this runs Qt's font matching algorithm
	QString found_family(QFontInfo(font).family());
	this->defaultmap["editor/fontfamily"] = found_family;
 	this->defaultmap["editor/fontsize"] = 12;
	this->defaultmap["editor/syntaxhighlight"] = "For Light Background";

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
	addPrefPage(group, prefsAction3DView, page3DView);
	addPrefPage(group, prefsActionEditor, pageEditor);
#if defined(OPENSCAD_DEPLOY) && defined(Q_OS_MAC)
	addPrefPage(group, prefsActionUpdate, pageUpdate);
#else
	this->toolBar->removeAction(prefsActionUpdate);
#endif
#ifdef ENABLE_EXPERIMENTAL
	addPrefPage(group, prefsActionFeatures, pageFeatures);
#else
	this->toolBar->removeAction(prefsActionFeatures);
#endif
	addPrefPage(group, prefsActionAdvanced, pageAdvanced);
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

	setupFeaturesPage();
	updateGUI();

	RenderSettings::inst()->setColors(this->colorschemes[getValue("3dview/colorscheme").toString()]);
}

Preferences::~Preferences()
{
	removeDefaultSettings();
}

/**
 * Add a page for the preferences GUI. This handles both the action grouping
 * and the registration of the widget for each action to have a generalized
 * callback to switch pages.
 * 
 * @param group The action group for all page actions. This one will have the
 *              callback attached after creating all actions/pages.
 * @param action The action specific for the added page.
 * @param widget The widget that should be shown when the action is triggered.
 *               This must be a child page of the stackedWidget.
 */
void
Preferences::addPrefPage(QActionGroup *group, QAction *action, QWidget *widget)
{
	group->addAction(action);
	prefPages[action] = widget;
}

/**
 * Callback to switch pages in the preferences GUI.
 * 
 * @param action The action triggered by the user.
 */
void
Preferences::actionTriggered(QAction *action)
{
	this->stackedWidget->setCurrentWidget(prefPages[action]);
}

/**
 * Callback for the dynamically created checkboxes on the features
 * page. The specific Feature object is associated as property with
 * the callback.
 * 
 * @param state the state of the checkbox.
 */
void Preferences::featuresCheckBoxToggled(bool state)
{
	const QObject *sender = QObject::sender();
	if (sender == NULL) {
		return;
	}
	QVariant v = sender->property(featurePropertyName);
	if (!v.isValid()) {
		return;
	}
	Feature *feature = v.value<Feature *>();
	feature->enable(state);
	QSettings settings;
	settings.setValue(QString("feature/%1").arg(QString::fromStdString(feature->get_name())), state);
}

/**
 * Setup feature GUI and synchronize the Qt settings with the feature values.
 * 
 * When running in GUI mode, the feature setting that might have been set
 * from commandline is ignored. This always uses the value coming from the
 * QSettings.
 */
void
Preferences::setupFeaturesPage()
{
	int row = 0;
	for (Feature::iterator it = Feature::begin();it != Feature::end();it++) {
		Feature *feature = *it;
		
		QString featurekey = QString("feature/%1").arg(QString::fromStdString(feature->get_name()));
		this->defaultmap[featurekey] = false;

		// spacer item between the features, just for some optical separation
		gridLayoutExperimentalFeatures->addItem(new QSpacerItem(1, 8, QSizePolicy::Expanding, QSizePolicy::Fixed), row, 1, 1, 1, Qt::AlignCenter);
		row++;

		QCheckBox *cb = new QCheckBox(QString::fromStdString(feature->get_name()), pageFeatures);
		QFont bold_font(cb->font());
		bold_font.setBold(true);
		cb->setFont(bold_font);
		// synchronize Qt settings with the feature settings
		bool value = getValue(featurekey).toBool();
		feature->enable(value);
		cb->setChecked(value);
		cb->setProperty(featurePropertyName, QVariant::fromValue<Feature *>(feature));
		connect(cb, SIGNAL(toggled(bool)), this, SLOT(featuresCheckBoxToggled(bool)));		
		gridLayoutExperimentalFeatures->addWidget(cb, row, 0, 1, 2, Qt::AlignLeading);
		row++;
		
		QLabel *l = new QLabel(QString::fromStdString(feature->get_description()), pageFeatures);
		l->setTextFormat(Qt::RichText);
		gridLayoutExperimentalFeatures->addWidget(l, row, 1, 1, 1, Qt::AlignLeading);
		row++;
	}
	// Force fixed indentation, the checkboxes use column span of 2 so 
	// first row is not constrained in size by the visible controls. The
	// fixed size space essentially gives the first row the width of the
	// spacer item itself.
	gridLayoutExperimentalFeatures->addItem(new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0, 1, 1, Qt::AlignLeading);
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

void Preferences::on_syntaxHighlight_currentIndexChanged(const QString &s)
{
	QSettings settings;
	settings.setValue("editor/syntaxhighlight", s);
	emit syntaxHighlightChanged(s);
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
#ifdef Q_OS_MAC
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

	QString shighlight = getValue("editor/syntaxhighlight").toString();
	int shidx = this->syntaxHighlight->findText(shighlight);
	if (shidx >= 0) this->syntaxHighlight->setCurrentIndex(shidx);

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
