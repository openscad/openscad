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
#include <boost/algorithm/string.hpp>
#include "GeometryCache.h"
#include "AutoUpdater.h"
#include "feature.h"
#ifdef ENABLE_CGAL
#include "CGALCache.h"
#endif
#include "colormap.h"
#include "rendersettings.h"

Preferences *Preferences::instance = NULL;

const char * Preferences::featurePropertyName = "FeatureProperty";
Q_DECLARE_METATYPE(Feature *);

class SettingsReader : public Settings::SettingsVisitor
{
    QSettings settings;
    Value getValue(const Settings::SettingsEntry& entry, const std::string& value) const {
	std::string trimmed_value(value);
	boost::trim(trimmed_value);

	if (trimmed_value.empty()) {
		return entry.defaultValue();
	}

	try {
		switch (entry.defaultValue().type()) {
		case Value::STRING:
			return Value(trimmed_value);
		case Value::NUMBER:
			return Value(boost::lexical_cast<int>(trimmed_value));
		case Value::BOOL:
			boost::to_lower(trimmed_value);
			if ("false" == trimmed_value) {
				return Value(false);
			} else if ("true" == trimmed_value) {
				return Value(true);
			}
			return Value(boost::lexical_cast<bool>(trimmed_value));
		default:
			assert(false && "invalid value type for settings");
		}
	} catch (const boost::bad_lexical_cast& e) {
		return entry.defaultValue();
	}
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

	QSettings settings;
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
		if (size == savedsize) {
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
#else
	this->toolBar->removeAction(prefsActionFeatures);
#endif
	addPrefPage(group, prefsActionAdvanced, pageAdvanced);
	connect(group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

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

	SettingsReader settingsReader;
	Settings::Settings::inst()->visit(settingsReader);
	emit editorConfigChanged();
}

Preferences::~Preferences()
{
	removeDefaultSettings();
	instance = NULL;
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
	emit ExperimentalChanged();
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
	emit colorSchemeChanged( scheme );
}

void Preferences::on_fontChooser_activated(const QString &family)
{
	QSettings settings;
	settings.setValue("editor/fontfamily", family);
	emit fontChanged(family, getValue("editor/fontsize").toUInt());
}

void Preferences::on_fontSize_currentIndexChanged(const QString &size)
{
	uint intsize = size.toUInt();
	QSettings settings;
	settings.setValue("editor/fontsize", intsize);
	emit fontChanged(getValue("editor/fontfamily").toString(), intsize);
}

void Preferences::on_editorType_currentIndexChanged(const QString &type)
{
	QSettings settings;
	settings.setValue("editor/editortype", type);
}

void Preferences::on_syntaxHighlight_activated(const QString &s)
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
	if (AutoUpdater *updater = AutoUpdater::updater()) {
		updater->checkForUpdates();
	} else {
		unimplemented_msg();
	}
}

void
Preferences::on_mdiCheckBox_toggled(bool state)
{
	QSettings settings;
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
	QSettings settings;
	settings.setValue("advanced/reorderWindows", state);
	emit updateReorderMode(state);
}

void
Preferences::on_undockCheckBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/undockableWindows", state);
	emit updateUndockMode(state);
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
	GeometryCache::instance()->setMaxSize(text.toULong());
}

void Preferences::on_opencsgLimitEdit_textChanged(const QString &text)
{
	QSettings settings;
	settings.setValue("advanced/openCSGLimit", text);
	// FIXME: Set this globally?
}

void Preferences::on_localizationCheckBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/localization", state);
}

void Preferences::on_forceGoldfeatherBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("advanced/forceGoldfeather", state);
	emit openCSGSettingsChanged();
}

void Preferences::on_mouseWheelZoomBox_toggled(bool state)
{
	QSettings settings;
	settings.setValue("editor/ctrlmousewheelzoom", state);
}

void Preferences::on_launcherBox_toggled(bool state)
{
	QSettings settings;
 	settings.setValue("launcher/showOnStartup", state);	
}

void Preferences::on_checkBoxShowWarningsIn3dView_toggled(bool val)
{
	Settings::Settings::inst()->set(Settings::Settings::showWarningsIn3dView, Value(val));
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
    if (instance != NULL) {
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

Preferences *Preferences::inst() {
    assert(instance != NULL);
    
    return instance;
}
