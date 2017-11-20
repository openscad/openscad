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
#include <QStatusBar>
#include <QSettings>
#include <boost/algorithm/string.hpp>
#include "GeometryCache.h"
#include "AutoUpdater.h"
#include "feature.h"
#ifdef ENABLE_CGAL
#include "CGALCache.h"
#endif
#include "colormap.h"
#include "rendersettings.h"
#include "QSettingsCached.h"
#include "input/InputDriverManager.h"

Preferences *Preferences::instance = nullptr;

const char * Preferences::featurePropertyName = "FeatureProperty";
Q_DECLARE_METATYPE(Feature *);

class SettingsReader : public Settings::SettingsVisitor
{
    QSettingsCached settings;
    Value getValue(const Settings::SettingsEntry& entry, const std::string& value) const {
	std::string trimmed_value(value);
	boost::trim(trimmed_value);

	if (trimmed_value.empty()) {
		return entry.defaultValue();
	}

	try {
		switch (entry.defaultValue().type()) {
		case Value::ValueType::STRING:
			if(entry.defaultValue()=="0.00" || entry.defaultValue()=="0.10"|| entry.defaultValue()=="1.00"){ //ToDo: Clean me up
				return Value(boost::lexical_cast<double>(trimmed_value));
			}
			return Value(trimmed_value);
		case Value::ValueType::NUMBER: //ToDo: Clean me up - Number is actually a double
			return Value(boost::lexical_cast<int>(trimmed_value));
		case Value::ValueType::BOOL:
			boost::to_lower(trimmed_value);
			if ("false" == trimmed_value) {
				return Value(false);
			} else if ("true" == trimmed_value) {
				return Value(true);
			}
			return Value(boost::lexical_cast<bool>(trimmed_value));
		default:
			assert(false && "invalid value type for settings");
			return 0; // keep compiler happy
		}
	} catch (const boost::bad_lexical_cast& e) {
		return entry.defaultValue();
	}
	return entry.defaultValue();
    }

    virtual void handle(Settings::SettingsEntry& entry) const {
	Settings::Settings *s = Settings::Settings::inst();

	std::string key = entry.category() + "/" + entry.name();
	std::string value = settings.value(QString::fromStdString(key)).toString().toStdString();
	const Value v = getValue(entry, value);
	PRINTDB("SettingsReader R: %s = '%s' => '%s'", key.c_str() % value.c_str() % v.toString());
	s->set(entry, v);
    }
};

class SettingsWriter : public Settings::SettingsVisitor
{
    virtual void handle(Settings::SettingsEntry& entry) const {
	Settings::Settings *s = Settings::Settings::inst();

	QSettingsCached settings;
	QString key = QString::fromStdString(entry.category() + "/" + entry.name());
	if (entry.is_default()) {
	    settings.remove(key);
	    PRINTDB("SettingsWriter D: %s", key.toStdString().c_str());
	} else {
	    const Value &value = s->get(entry);
	    settings.setValue(key, QString::fromStdString(value.toString()));
	    PRINTDB("SettingsWriter W: %s = '%s'", key.toStdString().c_str() % value.toString().c_str());
	}
    }
};

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
	setupUi(this);
}

void Preferences::init() {
	
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
	this->defaultmap["editor/editortype"] = "QScintilla Editor";

#if defined (Q_OS_MAC)
	this->defaultmap["editor/ctrlmousewheelzoom"] = false;
#else
	this->defaultmap["editor/ctrlmousewheelzoom"] = true;
#endif

	uint savedsize = getValue("editor/fontsize").toUInt();
	QFontDatabase db;
	for(auto size : db.standardSizes()) {
		this->fontSize->addItem(QString::number(size));
		if (static_cast<uint>(size) == savedsize) {
			this->fontSize->setCurrentIndex(this->fontSize->count()-1);
		}
	}

	// reset GUI fontsize if fontSize->addItem emitted signals that changed it.
	this->fontSize->setEditText( QString("%1").arg( savedsize ) );
	
	// Setup default settings
	this->defaultmap["advanced/opencsg_show_warning"] = true;
	this->defaultmap["advanced/enable_opencsg_opengl1x"] = true;
	this->defaultmap["advanced/polysetCacheSize"] = uint(GeometryCache::instance()->maxSize());
#ifdef ENABLE_CGAL
	this->defaultmap["advanced/cgalCacheSize"] = uint(CGALCache::instance()->maxSize());
#endif
	this->defaultmap["advanced/openCSGLimit"] = RenderSettings::inst()->openCSGTermLimit;
	this->defaultmap["advanced/forceGoldfeather"] = false;
	this->defaultmap["advanced/mdi"] = true;
	this->defaultmap["advanced/undockableWindows"] = false;
	this->defaultmap["advanced/reorderWindows"] = true;
	this->defaultmap["launcher/showOnStartup"] = true;
	this->defaultmap["advanced/localization"] = true;

	// Toolbar
	QActionGroup *group = new QActionGroup(this);
	addPrefPage(group, prefsAction3DView, page3DView);
	addPrefPage(group, prefsActionEditor, pageEditor);
#ifdef OPENSCAD_UPDATER
	addPrefPage(group, prefsActionUpdate, pageUpdate);
#else
	this->toolBar->removeAction(prefsActionUpdate);
#endif
#ifdef ENABLE_EXPERIMENTAL
	addPrefPage(group, prefsActionFeatures, pageFeatures);
	addPrefPage(group, prefsActionInput, pageInput);
	addPrefPage(group, prefsActionInputButton, pageInputButton);
#else
	this->toolBar->removeAction(prefsActionFeatures);
	this->toolBar->removeAction(prefsActionInput);
	this->toolBar->removeAction(prefsActionInputButton);
#endif
	addPrefPage(group, prefsActionAdvanced, pageAdvanced);
	
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));
	connect(this->pushButtonAxisTrim, SIGNAL(clicked()), this, SLOT(on_AxisTrim()));
	connect(this->pushButtonAxisTrimReset, SIGNAL(clicked()), this, SLOT(on_AxisTrimReset()));

	prefsAction3DView->setChecked(true);
	this->actionTriggered(this->prefsAction3DView);

	// 3D View pane
	this->defaultmap["3dview/colorscheme"] = "Cornfield";

  // Advanced pane	
	QValidator *validator = new QIntValidator(this);
#ifdef ENABLE_CGAL
	this->cgalCacheSizeEdit->setValidator(validator);
#endif
	this->polysetCacheSizeEdit->setValidator(validator);
	this->opencsgLimitEdit->setValidator(validator);

	initComboBox(this->comboBoxIndentUsing, Settings::Settings::indentStyle);
	initComboBox(this->comboBoxLineWrap, Settings::Settings::lineWrap);
	initComboBox(this->comboBoxLineWrapIndentationStyle, Settings::Settings::lineWrapIndentationStyle);
	initComboBox(this->comboBoxLineWrapVisualizationEnd, Settings::Settings::lineWrapVisualizationEnd);
	initComboBox(this->comboBoxLineWrapVisualizationStart, Settings::Settings::lineWrapVisualizationBegin);
	initComboBox(this->comboBoxShowWhitespace, Settings::Settings::showWhitespace);
	initComboBox(this->comboBoxTabKeyFunction, Settings::Settings::tabKeyFunction);
	initSpinBox(this->spinBoxIndentationWidth, Settings::Settings::indentationWidth);
	initSpinBox(this->spinBoxLineWrapIndentationIndent, Settings::Settings::lineWrapIndentation);
	initSpinBox(this->spinBoxShowWhitespaceSize, Settings::Settings::showWhitespaceSize);
	initSpinBox(this->spinBoxTabWidth, Settings::Settings::tabWidth);

        initComboBox(this->comboBoxTranslationX, Settings::Settings::inputTranslationX);
        initComboBox(this->comboBoxTranslationY, Settings::Settings::inputTranslationY);
        initComboBox(this->comboBoxTranslationZ, Settings::Settings::inputTranslationZ);
        initComboBox(this->comboBoxTranslationXVPRel, Settings::Settings::inputTranslationXVPRel);
        initComboBox(this->comboBoxTranslationYVPRel, Settings::Settings::inputTranslationYVPRel);
        initComboBox(this->comboBoxTranslationZVPRel, Settings::Settings::inputTranslationZVPRel);
        initComboBox(this->comboBoxRotationX, Settings::Settings::inputRotateX);
        initComboBox(this->comboBoxRotationY, Settings::Settings::inputRotateY);
        initComboBox(this->comboBoxRotationZ, Settings::Settings::inputRotateZ);
        initComboBox(this->comboBoxZoom, Settings::Settings::inputZoom);

		for (int i = 0; i < InputEventMapper::getMaxButtons(); i++ ){
			std::string s = std::to_string(i);
			QComboBox* box = this->centralwidget->findChild<QComboBox *>(QString::fromStdString("comboBoxButton"+s));
			Settings::SettingsEntry* ent = Settings::Settings::inst()->getSettingEntryByName("button" +s );
			if(box && ent){
				initComboBox(box,*ent);
			}
		}

		for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
			std::string s = std::to_string(i);

			QDoubleSpinBox* spin;
			Settings::SettingsEntry* ent;

			spin = this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxTrim"+s));
			ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);
			if(spin && ent){
				initDoubleSpinBox(spin,*ent);
			}
			spin = this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxDeadzone"+s));
			ent = Settings::Settings::inst()->getSettingEntryByName("axisDeadzone" +s);
			if(spin && ent){
				initDoubleSpinBox(spin,*ent);
			}
		}

	initDoubleSpinBox(this->doubleSpinBoxTranslationGain, Settings::Settings::inputTranslationGain);
	initDoubleSpinBox(this->doubleSpinBoxTranslationVPRelGain, Settings::Settings::inputTranslationVPRelGain);
	initDoubleSpinBox(this->doubleSpinBoxRotateGain, Settings::Settings::inputRotateGain);
	initDoubleSpinBox(this->doubleSpinBoxZoomGain, Settings::Settings::inputZoomGain);

	SettingsReader settingsReader;
	Settings::Settings::inst()->visit(settingsReader);
	emit editorConfigChanged();
}

Preferences::~Preferences()
{
	removeDefaultSettings();
	instance = nullptr;
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
	if (sender == nullptr) {
		return;
	}
	QVariant v = sender->property(featurePropertyName);
	if (!v.isValid()) {
		return;
	}
	Feature *feature = v.value<Feature *>();
	feature->enable(state);
	QSettingsCached settings;
	settings.setValue(QString("feature/%1").arg(QString::fromStdString(feature->get_name())), state);
	emit ExperimentalChanged();

	if (!Feature::ExperimentalInputDriver.is_enabled()) {
		this->toolBar->removeAction(prefsActionInput);
		this->toolBar->removeAction(prefsActionInputButton);
		InputDriverManager::instance()->closeDrivers();
	}
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
	QSettingsCached settings;
	settings.setValue("3dview/colorscheme", scheme);
	emit colorSchemeChanged( scheme );
}

void Preferences::on_fontChooser_activated(const QString &family)
{
	QSettingsCached settings;
	settings.setValue("editor/fontfamily", family);
	emit fontChanged(family, getValue("editor/fontsize").toUInt());
}

void Preferences::on_fontSize_currentIndexChanged(const QString &size)
{
	uint intsize = size.toUInt();
	QSettingsCached settings;
	settings.setValue("editor/fontsize", intsize);
	emit fontChanged(getValue("editor/fontfamily").toString(), intsize);
}

void Preferences::on_editorType_currentIndexChanged(const QString &type)
{
	QSettingsCached settings;
	settings.setValue("editor/editortype", type);
}

void Preferences::on_syntaxHighlight_activated(const QString &s)
{
	QSettingsCached settings;
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
	if (AutoUpdater *updater = AutoUpdater::updater()) {
		updater->checkForUpdates();
	} else {
		unimplemented_msg();
	}
}

void
Preferences::on_mdiCheckBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/mdi", state);
	emit updateMdiMode(state);
}

void
Preferences::on_reorderCheckBox_toggled(bool state)
{
	if (!state) {
		undockCheckBox->setChecked(false);
	}
	undockCheckBox->setEnabled(state);
	QSettingsCached settings;
	settings.setValue("advanced/reorderWindows", state);
	emit updateReorderMode(state);
}

void
Preferences::on_undockCheckBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/undockableWindows", state);
	emit updateUndockMode(state);
}

void
Preferences::on_openCSGWarningBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/opencsg_show_warning",state);
}

void
Preferences::on_enableOpenCSGBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/enable_opencsg_opengl1x", state);
}

void Preferences::on_cgalCacheSizeEdit_textChanged(const QString &text)
{
	QSettingsCached settings;
	settings.setValue("advanced/cgalCacheSize", text);
#ifdef ENABLE_CGAL
	CGALCache::instance()->setMaxSize(text.toULong());
#endif
}

void Preferences::on_polysetCacheSizeEdit_textChanged(const QString &text)
{
	QSettingsCached settings;
	settings.setValue("advanced/polysetCacheSize", text);
	GeometryCache::instance()->setMaxSize(text.toULong());
}

void Preferences::on_opencsgLimitEdit_textChanged(const QString &text)
{
	QSettingsCached settings;
	settings.setValue("advanced/openCSGLimit", text);
	// FIXME: Set this globally?
}

void Preferences::on_localizationCheckBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/localization", state);
}

void Preferences::on_forceGoldfeatherBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("advanced/forceGoldfeather", state);
	emit openCSGSettingsChanged();
}

void Preferences::on_mouseWheelZoomBox_toggled(bool state)
{
	QSettingsCached settings;
	settings.setValue("editor/ctrlmousewheelzoom", state);
}

void Preferences::on_launcherBox_toggled(bool state)
{
	QSettingsCached settings;
 	settings.setValue("launcher/showOnStartup", state);	
}

void Preferences::on_checkBoxShowWarningsIn3dView_toggled(bool val)
{
	Settings::Settings::inst()->set(Settings::Settings::showWarningsIn3dView, Value(val));
	writeSettings();
}

void Preferences::on_AxisTrim()
{
	InputEventMapper::instance()->onAxisAutoTrim();

	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);

		QDoubleSpinBox* spin;
		Settings::SettingsEntry* ent;

		spin = this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxTrim"+s));
		ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);

		if(spin && ent){
			spin->setValue((double)Settings::Settings::inst()->get(*ent).toDouble());
		}
	}
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_AxisTrimReset()
{
	InputEventMapper::instance()->onAxisTrimReset();
	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);

		QDoubleSpinBox* spin;
		Settings::SettingsEntry* ent;

		ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s);
		if(ent){
			Settings::Settings::inst()->set(*ent, 0.00);
		}

		spin = this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxTrim"+s));
		if(spin){
			spin->setValue(0.00);
		}
	}
	emit inputCalibrationChanged();
	writeSettings();
}


void Preferences::on_spinBoxIndentationWidth_valueChanged(int val)
{
	Settings::Settings::inst()->set(Settings::Settings::indentationWidth, Value(val));
	writeSettings();
}

void Preferences::on_spinBoxTabWidth_valueChanged(int val)
{
	Settings::Settings::inst()->set(Settings::Settings::tabWidth, Value(val));
	writeSettings();
}

void Preferences::on_comboBoxLineWrap_activated(int val)
{
	applyComboBox(comboBoxLineWrap, val, Settings::Settings::lineWrap);
}

void Preferences::on_comboBoxLineWrapIndentationStyle_activated(int val)
{
	spinBoxLineWrapIndentationIndent->setDisabled(comboBoxLineWrapIndentationStyle->currentText() == "Same");
	applyComboBox(comboBoxLineWrapIndentationStyle, val, Settings::Settings::lineWrapIndentationStyle);
}

void Preferences::on_spinBoxLineWrapIndentationIndent_valueChanged(int val)
{
	Settings::Settings::inst()->set(Settings::Settings::lineWrapIndentation, Value(val));
	writeSettings();
}

void Preferences::on_comboBoxLineWrapVisualizationStart_activated(int val)
{
	applyComboBox(comboBoxLineWrapVisualizationStart, val, Settings::Settings::lineWrapVisualizationBegin);
}

void Preferences::on_comboBoxLineWrapVisualizationEnd_activated(int val)
{
	applyComboBox(comboBoxLineWrapVisualizationEnd, val, Settings::Settings::lineWrapVisualizationEnd);
}

void Preferences::on_comboBoxShowWhitespace_activated(int val)
{
	applyComboBox(comboBoxShowWhitespace, val, Settings::Settings::showWhitespace);
}

void Preferences::on_spinBoxShowWhitespaceSize_valueChanged(int val)
{
	Settings::Settings::inst()->set(Settings::Settings::showWhitespaceSize, Value(val));
	writeSettings();
}

void Preferences::on_checkBoxAutoIndent_toggled(bool val)
{
	Settings::Settings::inst()->set(Settings::Settings::autoIndent, Value(val));
	writeSettings();
}

void Preferences::on_checkBoxBackspaceUnindents_toggled(bool val)
{
    Settings::Settings::inst()->set(Settings::Settings::backspaceUnindents, Value(val));
    writeSettings();
}

void Preferences::on_comboBoxIndentUsing_activated(int val)
{
	applyComboBox(comboBoxIndentUsing, val, Settings::Settings::indentStyle);
}

void Preferences::on_comboBoxTabKeyFunction_activated(int val)
{
	applyComboBox(comboBoxTabKeyFunction, val, Settings::Settings::tabKeyFunction);
}

void Preferences::on_checkBoxHighlightCurrentLine_toggled(bool val)
{
	Settings::Settings::inst()->set(Settings::Settings::highlightCurrentLine, Value(val));
	writeSettings();
}

void Preferences::on_checkBoxEnableBraceMatching_toggled(bool val)
{
	Settings::Settings::inst()->set(Settings::Settings::enableBraceMatching, Value(val));
	writeSettings();
}

void Preferences::on_checkBoxEnableLineNumbers_toggled(bool checked)
{
	Settings::Settings::inst()->set(Settings::Settings::enableLineNumbers, Value(checked));
	writeSettings();
}

void Preferences::on_comboBoxTranslationX_activated(int val)
{
	applyComboBox(comboBoxTranslationX, val, Settings::Settings::inputTranslationX);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxTranslationY_activated(int val)
{
	applyComboBox(comboBoxTranslationY, val, Settings::Settings::inputTranslationY);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxTranslationZ_activated(int val)
{
	applyComboBox(comboBoxTranslationZ, val, Settings::Settings::inputTranslationZ);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxTranslationXVPRel_activated(int val)
{
	applyComboBox(comboBoxTranslationXVPRel, val, Settings::Settings::inputTranslationXVPRel);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxTranslationYVPRel_activated(int val)
{
	applyComboBox(comboBoxTranslationYVPRel, val, Settings::Settings::inputTranslationYVPRel);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxTranslationZVPRel_activated(int val)
{
	applyComboBox(comboBoxTranslationZVPRel, val, Settings::Settings::inputTranslationZVPRel);
        emit inputMappingChanged();
}
void Preferences::on_comboBoxRotationX_activated(int val)
{
	applyComboBox(comboBoxRotationX, val, Settings::Settings::inputRotateX);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxRotationY_activated(int val)
{
	applyComboBox(comboBoxRotationY, val, Settings::Settings::inputRotateY);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxRotationZ_activated(int val)
{
	applyComboBox(comboBoxRotationZ, val, Settings::Settings::inputRotateZ);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxZoom_activated(int val)
{
	applyComboBox(comboBoxZoom, val, Settings::Settings::inputZoom);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton0_activated(int val)
{
	applyComboBox(comboBoxButton0, val, Settings::Settings::inputButton0);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton1_activated(int val)
{
	applyComboBox(comboBoxButton1, val, Settings::Settings::inputButton1);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton2_activated(int val)
{
	applyComboBox(comboBoxButton2, val, Settings::Settings::inputButton2);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton3_activated(int val)
{
	applyComboBox(comboBoxButton3, val, Settings::Settings::inputButton3);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton4_activated(int val)
{
	applyComboBox(comboBoxButton4, val, Settings::Settings::inputButton4);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton5_activated(int val)
{
	applyComboBox(comboBoxButton5, val, Settings::Settings::inputButton5);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton6_activated(int val)
{
	applyComboBox(comboBoxButton6, val, Settings::Settings::inputButton6);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton7_activated(int val)
{
	applyComboBox(comboBoxButton7, val, Settings::Settings::inputButton7);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton8_activated(int val)
{
	applyComboBox(comboBoxButton8, val, Settings::Settings::inputButton8);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton9_activated(int val)
{
	applyComboBox(comboBoxButton9, val, Settings::Settings::inputButton9);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton10_activated(int val)
{
	applyComboBox(comboBoxButton10, val, Settings::Settings::inputButton10);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton11_activated(int val)
{
	applyComboBox(comboBoxButton11, val, Settings::Settings::inputButton11);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton12_activated(int val)
{
	applyComboBox(comboBoxButton12, val, Settings::Settings::inputButton12);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton13_activated(int val)
{
	applyComboBox(comboBoxButton13, val, Settings::Settings::inputButton13);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton14_activated(int val)
{
	applyComboBox(comboBoxButton14, val, Settings::Settings::inputButton14);
        emit inputMappingChanged();
}

void Preferences::on_comboBoxButton15_activated(int val)
{
	applyComboBox(comboBoxButton15, val, Settings::Settings::inputButton15);
        emit inputMappingChanged();
}


void Preferences::on_doubleSpinBoxTrim0_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim0, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim1_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim1, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim2_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim2, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim3_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim3, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim4_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim4, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim5_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim5, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim6_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim6, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTrim7_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim7, Value(val));
	emit inputCalibrationChanged();
}

void Preferences::on_doubleSpinBoxTrim8_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisTrim8, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone0_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone0, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone1_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone1, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone2_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone2, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone3_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone3, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone4_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone4, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone5_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone5, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone6_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone6, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone7_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone7, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxDeadzone8_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::axisDeadzone8, Value(val));
	emit inputCalibrationChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxRotateGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputRotateGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTranslationGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputTranslationGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxTranslationVPRelGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputTranslationVPRelGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void Preferences::on_doubleSpinBoxZoomGain_valueChanged(double val)
{
	Settings::Settings::inst()->set(Settings::Settings::inputZoomGain, Value(val));
	emit inputGainChanged();
	writeSettings();
}

void Preferences::writeSettings()
{
	SettingsWriter settingsWriter;
	Settings::Settings::inst()->visit(settingsWriter);
	fireEditorConfigChanged();
}

void Preferences::fireEditorConfigChanged() const
{
	emit editorConfigChanged();
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
	QSettingsCached settings;
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
	QSettingsCached settings;
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
	if (shidx >= 0) {
	    this->syntaxHighlight->setCurrentIndex(shidx);
	} else {
	    int offidx = this->syntaxHighlight->findText("Off");
	    if (offidx >= 0) {
		this->syntaxHighlight->setCurrentIndex(offidx);
	    }
	}

	QString editortypevar = getValue("editor/editortype").toString();
	int edidx = this->editorType->findText(editortypevar);
	if (edidx >=0) this->editorType->setCurrentIndex(edidx);

	this->mouseWheelZoomBox->setChecked(getValue("editor/ctrlmousewheelzoom").toBool());

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
	this->localizationCheckBox->setChecked(getValue("advanced/localization").toBool());
	this->forceGoldfeatherBox->setChecked(getValue("advanced/forceGoldfeather").toBool());
	this->mdiCheckBox->setChecked(getValue("advanced/mdi").toBool());
	this->reorderCheckBox->setChecked(getValue("advanced/reorderWindows").toBool());
	this->undockCheckBox->setChecked(getValue("advanced/undockableWindows").toBool());
	this->undockCheckBox->setEnabled(this->reorderCheckBox->isChecked());
	this->launcherBox->setChecked(getValue("launcher/showOnStartup").toBool());

	Settings::Settings *s = Settings::Settings::inst();
	updateComboBox(this->comboBoxLineWrap, Settings::Settings::lineWrap);
	updateComboBox(this->comboBoxLineWrapIndentationStyle, Settings::Settings::lineWrapIndentationStyle);
	updateComboBox(this->comboBoxLineWrapVisualizationStart, Settings::Settings::lineWrapVisualizationBegin);
	updateComboBox(this->comboBoxLineWrapVisualizationEnd, Settings::Settings::lineWrapVisualizationEnd);
	updateComboBox(this->comboBoxShowWhitespace, Settings::Settings::showWhitespace);
	updateComboBox(this->comboBoxIndentUsing, Settings::Settings::indentStyle);
	updateComboBox(this->comboBoxTabKeyFunction, Settings::Settings::tabKeyFunction);
	this->spinBoxIndentationWidth->setValue(s->get(Settings::Settings::indentationWidth).toDouble());
	this->spinBoxTabWidth->setValue(s->get(Settings::Settings::tabWidth).toDouble());
	this->spinBoxLineWrapIndentationIndent->setValue(s->get(Settings::Settings::lineWrapIndentation).toDouble());
	this->spinBoxShowWhitespaceSize->setValue(s->get(Settings::Settings::showWhitespaceSize).toDouble());
	this->checkBoxAutoIndent->setChecked(s->get(Settings::Settings::autoIndent).toBool());
	this->checkBoxBackspaceUnindents->setChecked(s->get(Settings::Settings::backspaceUnindents).toBool());
	this->checkBoxHighlightCurrentLine->setChecked(s->get(Settings::Settings::highlightCurrentLine).toBool());
	this->checkBoxEnableBraceMatching->setChecked(s->get(Settings::Settings::enableBraceMatching).toBool());
	this->checkBoxShowWarningsIn3dView->setChecked(s->get(Settings::Settings::showWarningsIn3dView).toBool());
	this->checkBoxEnableLineNumbers->setChecked(s->get(Settings::Settings::enableLineNumbers).toBool());
	this->spinBoxLineWrapIndentationIndent->setDisabled(this->comboBoxLineWrapIndentationStyle->currentText() == "Same");

	updateComboBox(this->comboBoxTranslationX, Settings::Settings::inputTranslationX);
	updateComboBox(this->comboBoxTranslationY, Settings::Settings::inputTranslationY);
	updateComboBox(this->comboBoxTranslationZ, Settings::Settings::inputTranslationZ);
	updateComboBox(this->comboBoxTranslationXVPRel, Settings::Settings::inputTranslationXVPRel);
	updateComboBox(this->comboBoxTranslationYVPRel, Settings::Settings::inputTranslationYVPRel);
	updateComboBox(this->comboBoxTranslationZVPRel, Settings::Settings::inputTranslationZVPRel);
	updateComboBox(this->comboBoxRotationX, Settings::Settings::inputRotateX);
	updateComboBox(this->comboBoxRotationY, Settings::Settings::inputRotateY);
	updateComboBox(this->comboBoxRotationZ, Settings::Settings::inputRotateZ);
	updateComboBox(this->comboBoxZoom, Settings::Settings::inputZoom);

	for (int i = 0; i < InputEventMapper::getMaxButtons(); i++ ){
		std::string s = std::to_string(i);
		QComboBox* box = this->centralwidget->findChild<QComboBox *>(QString::fromStdString("comboBoxButton"+s));
		Settings::SettingsEntry* ent = Settings::Settings::inst()->getSettingEntryByName("button" +s );
		if(box && ent){
			updateComboBox(box,*ent);
		}
	}

	for (int i = 0; i < InputEventMapper::getMaxAxis(); i++ ){
		std::string s = std::to_string(i);
		Settings::Settings *setting = Settings::Settings::inst();

		QDoubleSpinBox* spin;
		Settings::SettingsEntry* ent;

		spin= this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxTrim"+s));
		ent = Settings::Settings::inst()->getSettingEntryByName("axisTrim" +s );
		if(spin && ent){
			spin->setValue((double)setting->get(*ent).toDouble());
		}

		spin= this->centralwidget->findChild<QDoubleSpinBox *>(QString::fromStdString("doubleSpinBoxDeadzone"+s));
		ent = Settings::Settings::inst()->getSettingEntryByName("axisDeadzone" +s );
		if(spin && ent){
			spin->setValue((double)setting->get(*ent).toDouble());
		}
	}

	this->doubleSpinBoxRotateGain->setValue((double)s->get(Settings::Settings::inputRotateGain).toDouble());
	this->doubleSpinBoxTranslationGain->setValue((double)s->get(Settings::Settings::inputTranslationGain).toDouble());
	this->doubleSpinBoxTranslationVPRelGain->setValue((double)s->get(Settings::Settings::inputTranslationVPRelGain).toDouble());
	this->doubleSpinBoxZoomGain->setValue((double)s->get(Settings::Settings::inputZoomGain).toDouble());
}

void Preferences::initComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	comboBox->clear();
	// Range is a vector of 2D vectors: [[name, value], ...]
	for(const auto &v : entry.range().toVector()) {
		QString val = QString::fromStdString(v[0]->toString());
		QString qtext = QString::fromStdString(gettext(v[1]->toString().c_str()));
		comboBox->addItem(qtext, val);
	}
}

void Preferences::initSpinBox(QSpinBox *spinBox, const Settings::SettingsEntry& entry)
{
	RangeType range = entry.range().toRange();
	spinBox->setMinimum(range.begin_value());
	spinBox->setMaximum(range.end_value());
}

void Preferences::initDoubleSpinBox(QDoubleSpinBox *spinBox, const Settings::SettingsEntry& entry)
{
	RangeType range = entry.range().toRange();
	spinBox->setMinimum(range.begin_value());
	spinBox->setMaximum(range.end_value());
}

void Preferences::updateComboBox(QComboBox *comboBox, const Settings::SettingsEntry& entry)
{
	Settings::Settings *s = Settings::Settings::inst();

	const Value &value = s->get(entry);
	QString text = QString::fromStdString(value.toString());
	int idx = comboBox->findData(text);
	if (idx >= 0) {
		comboBox->setCurrentIndex(idx);
	} else {
		const Value &defaultValue = entry.defaultValue();
		QString defaultText = QString::fromStdString(defaultValue.toString());
		int defIdx = comboBox->findData(defaultText);
		if (defIdx >= 0) {
			comboBox->setCurrentIndex(defIdx);
		} else {
			comboBox->setCurrentIndex(0);
		}
	}
}

void Preferences::applyComboBox(QComboBox *comboBox, int val, Settings::SettingsEntry& entry)
{
	QString s = comboBox->itemData(val).toString();
	Settings::Settings::inst()->set(entry, Value(s.toStdString()));
	writeSettings();
}

void Preferences::apply() const
{
	emit fontChanged(getValue("editor/fontfamily").toString(), getValue("editor/fontsize").toUInt());
	emit requestRedraw();
	emit openCSGSettingsChanged();
	emit syntaxHighlightChanged(getValue("editor/syntaxhighlight").toString());
}

void Preferences::create(QStringList colorSchemes)
{
    if (instance != nullptr) {
	return;
    }

    std::list<std::string> names = ColorMap::inst()->colorSchemeNames(true);
    QStringList renderColorSchemes;
    for(const auto &name : names) renderColorSchemes << name.c_str();
    
    instance = new Preferences();
    instance->syntaxHighlight->clear();
    instance->syntaxHighlight->addItems(colorSchemes);
    instance->colorSchemeChooser->clear();
    instance->colorSchemeChooser->addItems(renderColorSchemes);
    instance->init();
    instance->setupFeaturesPage();
    instance->updateGUI();
}

void Preferences::updateButtonState(int nr, bool pressed) const{
	QString Style = Preferences::EmptyString;
	if(pressed){
		Style=Preferences::ActiveStyleString;
	}
	std::string number = std::to_string(nr);

	QLabel* label = this->centralwidget->findChild<QLabel *>(QString::fromStdString("labelInputButton"+number));
	if(label==0) return;
	label->setStyleSheet(Style);
}

void Preferences::AxesChanged(int nr, double val) const{
	int value = val *100;

	QString s =  QString::number(val, 'f', 2 );
	std::string number = std::to_string(nr);
	QProgressBar* progressBar = this->centralwidget->findChild<QProgressBar *>(QString::fromStdString("progressBarAxis"+number));
	if(progressBar==0) return;
	progressBar->setValue(value);
	progressBar->setFormat(s);
}

Preferences *Preferences::inst() {
    assert(instance != nullptr);
    
    return instance;
}
