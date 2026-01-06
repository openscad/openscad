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

#include "gui/Preferences.h"

#include <unordered_map>
#include <vector>
#include <QFont>
#include <QFontComboBox>
#include <QMainWindow>
#include <QObject>
#include <QDialog>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <tuple>
#include <cassert>
#include <list>
#include <QMenu>
#include <QActionGroup>
#include <QMessageBox>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QFileDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStatusBar>
#include <QSettings>
#include <QTextDocument>
#include <QListWidget>
#include <QListWidgetItem>
#include <boost/algorithm/string.hpp>
#include "OctoPrintApiKeyDialog.h"
#include "geometry/GeometryCache.h"
#include "gui/AutoUpdater.h"
#include "Feature.h"
#include "core/Settings.h"
#include "utils/printutils.h"
#ifdef ENABLE_CGAL
#include "geometry/cgal/CGALCache.h"
#endif
#include "glview/ColorMap.h"
#include "glview/RenderSettings.h"
#include "gui/QSettingsCached.h"
#include "gui/SettingsWriter.h"
#include "gui/OctoPrint.h"
#include "gui/IgnoreWheelWhenNotFocused.h"
#include "gui/PrintService.h"

#include <string>

static const char *featurePropertyName = "FeatureProperty";

using S = Settings::Settings;

Q_DECLARE_METATYPE(Feature *);

class SettingsReader : public Settings::SettingsVisitor
{
  QSettingsCached settings;

  void handle(Settings::SettingsEntryBase& entry) const override
  {
    if (settings.contains(QString::fromStdString(entry.key()))) {
      std::string value = settings.value(QString::fromStdString(entry.key())).toString().toStdString();
      PRINTDB("SettingsReader R: %s = '%s'", entry.key() % value);
      entry.set(value);
    }
  }
};

Preferences::Preferences(QWidget *parent) : QMainWindow(parent)
{
  setupUi(this);

  std::list<std::string> names = ColorMap::inst()->colorSchemeNames(true);
  QStringList renderColorSchemes;
  for (const auto& name : names) renderColorSchemes << name.c_str();

  syntaxHighlight->clear();
  colorSchemeChooser->clear();
  colorSchemeChooser->addItems(renderColorSchemes);
  init();
  AxisConfig->init();
  setupFeaturesPage();
  setup3DPrintPage();
  updateGUI();
}

void Preferences::init()
{
  // Editor pane
  // Setup default font (Try to use a nice monospace font)
  const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  const QString found_family{QFontInfo{font}.family()};
  this->defaultmap["editor/fontfamily"] = found_family;
  this->defaultmap["editor/fontsize"] = 12;
  this->defaultmap["editor/syntaxhighlight"] = "For Light Background";

  const QFont applicationFont = QTextDocument().defaultFont();
  this->defaultmap["advanced/applicationFontFamily"] = applicationFont.family();
  this->defaultmap["advanced/applicationFontSize"] = applicationFont.pointSize();

  // Leave Console font with default if user has not chosen another.
  this->defaultmap["advanced/consoleFontFamily"] = applicationFont.family();
  this->defaultmap["advanced/consoleFontSize"] = applicationFont.pointSize();

  // Leave Customizer font with default if user has not chosen another.
  this->defaultmap["advanced/customizerFontFamily"] = applicationFont.family();
  this->defaultmap["advanced/customizerFontSize"] = applicationFont.pointSize();

#ifdef Q_OS_MACOS
  this->defaultmap["editor/ctrlmousewheelzoom"] = false;
#else
  this->defaultmap["editor/ctrlmousewheelzoom"] = true;
#endif

  createFontSizeMenu(fontSize, "editor/fontsize");
  createFontSizeMenu(comboBoxApplicationFontSize, "advanced/applicationFontSize");
  createFontSizeMenu(consoleFontSize, "advanced/consoleFontSize");
  createFontSizeMenu(customizerFontSize, "advanced/customizerFontSize");

  // Setup default settings
  this->defaultmap["advanced/opencsg_show_warning"] = true;
  this->defaultmap["advanced/polysetCacheSize"] =
    qulonglong(GeometryCache::instance()->maxSizeMB()) * 1024ul * 1024ul;
  this->defaultmap["advanced/polysetCacheSizeMB"] =
    getValue("advanced/polysetCacheSize").toULongLong() /
    (1024ul * 1024ul);  // carry over old settings if they exist
  this->defaultmap["advanced/cgalCacheSize"] =
    qulonglong(CGALCache::instance()->maxSizeMB()) * 1024ul * 1024ul;
  this->defaultmap["advanced/cgalCacheSizeMB"] =
    getValue("advanced/cgalCacheSize").toULongLong() /
    (1024ul * 1024ul);  // carry over old settings if they exist
  this->defaultmap["advanced/openCSGLimit"] = RenderSettings::inst()->openCSGTermLimit;
  this->defaultmap["advanced/forceGoldfeather"] = false;
  this->defaultmap["advanced/undockableWindows"] = false;
  this->defaultmap["advanced/reorderWindows"] = true;
  this->defaultmap["advanced/renderBackend3D"] =
    QString::fromStdString(renderBackend3DToString(RenderSettings::inst()->backend3D));
  this->defaultmap["launcher/showOnStartup"] = true;
  this->defaultmap["advanced/localization"] = true;
  this->defaultmap["advanced/autoReloadRaise"] = false;
  this->defaultmap["advanced/enableSoundNotification"] = true;
  this->defaultmap["advanced/timeThresholdOnRenderCompleteSound"] = 0;
  this->defaultmap["advanced/consoleMaxLines"] = 5000;
  this->defaultmap["advanced/consoleAutoClear"] = false;
  this->defaultmap["advanced/enableHardwarnings"] = false;
  this->defaultmap["advanced/traceDepth"] = 12;
  this->defaultmap["advanced/enableTraceUsermoduleParameters"] = true;
  this->defaultmap["advanced/enableParameterCheck"] = true;
  this->defaultmap["advanced/enableParameterRangeCheck"] = false;
  this->defaultmap["view/hideEditor"] = false;
  this->defaultmap["view/hideConsole"] = false;
  this->defaultmap["view/hideErrorLog"] = true;
  this->defaultmap["view/hideAnimate"] = true;
  this->defaultmap["view/hideCustomizer"] = true;
  this->defaultmap["view/hideFontList"] = true;
  this->defaultmap["view/hideColorList"] = true;
  this->defaultmap["view/hideViewportControl"] = true;
  this->defaultmap["editor/enableAutocomplete"] = true;
  this->defaultmap["editor/characterThreshold"] = 1;
  this->defaultmap["editor/stepSize"] = 1;

  // Toolbar
  auto *group = new QActionGroup(this);
  addPrefPage(group, prefsAction3DView, page3DView);
  addPrefPage(group, prefsActionEditor, pageEditor);
#ifdef OPENSCAD_UPDATER
  addPrefPage(group, prefsActionUpdate, pageUpdate);
#else
  this->toolBar->removeAction(prefsActionUpdate);
#endif
  addPrefPage(group, prefsAction3DPrint, page3DPrint);
#ifdef ENABLE_EXPERIMENTAL
  addPrefPage(group, prefsActionFeatures, pageFeatures);
#else
  this->toolBar->removeAction(prefsActionFeatures);
#endif
  addPrefPage(group, prefsActionInput, pageInput);
  addPrefPage(group, prefsActionInputButton, pageInputButton);
  addPrefPage(group, prefsActionMouse, pageMouse);
  addPrefPage(group, prefsActionAdvanced, pageAdvanced);
  addPrefPage(group, prefsActionDialogs, pageDialogs);

  connect(group, &QActionGroup::triggered, this, &Preferences::actionTriggered);

  prefsAction3DView->setChecked(true);
  this->actionTriggered(this->prefsAction3DView);

  // 3D View pane
  this->defaultmap["3dview/colorscheme"] = "Cornfield";

  // Advanced pane
  const int absolute_max = (sizeof(void *) == 8) ? 1024 * 1024 : 2048;  // 1TB for 64bit or 2GB for 32bit
  QValidator *memvalidator = new QIntValidator(1, absolute_max, this);
  auto *uintValidator = new QIntValidator(this);
  uintValidator->setBottom(0);
  QValidator *validator1 = new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,1}"),
                                                           this);  // range between 1-99 both inclusive
#ifdef ENABLE_CGAL
  this->cgalCacheSizeMBEdit->setValidator(memvalidator);
#endif
  this->polysetCacheSizeMBEdit->setValidator(memvalidator);
  this->opencsgLimitEdit->setValidator(uintValidator);
  this->timeThresholdOnRenderCompleteSoundEdit->setValidator(uintValidator);
  this->consoleMaxLinesEdit->setValidator(uintValidator);
  this->lineEditCharacterThreshold->setValidator(validator1);
  this->lineEditStepSize->setValidator(validator1);
  this->traceDepthEdit->setValidator(uintValidator);

  auto menu = new QMenu();
  menu->addAction(actionLocalAppParameterFile);
  menu->addAction(actionLocalAppParameterDir);
  menu->addAction(actionLocalAppParameterExtension);
  menu->addAction(actionLocalAppParameterSource);
  menu->addAction(actionLocalAppParameterSourceDir);
  toolButtonLocalAppParameterAddFile->setMenu(menu);

  Settings::Settings::visit(SettingsReader());

  initComboBox(this->comboBoxIndentUsing, Settings::Settings::indentStyle);
  initComboBox(this->comboBoxLineWrap, Settings::Settings::lineWrap);
  initComboBox(this->comboBoxLineWrapIndentationStyle, Settings::Settings::lineWrapIndentationStyle);
  initComboBox(this->comboBoxLineWrapVisualizationEnd, Settings::Settings::lineWrapVisualizationEnd);
  initComboBox(this->comboBoxLineWrapVisualizationStart, Settings::Settings::lineWrapVisualizationBegin);
  initComboBox(this->comboBoxShowWhitespace, Settings::Settings::showWhitespace);
  initComboBox(this->comboBoxModifierNumberScrollWheel, Settings::Settings::modifierNumberScrollWheel);
  initIntSpinBox(this->spinBoxIndentationWidth, Settings::Settings::indentationWidth);
  initIntSpinBox(this->spinBoxLineWrapIndentationIndent, Settings::Settings::lineWrapIndentation);
  initIntSpinBox(this->spinBoxShowWhitespaceSize, Settings::Settings::showWhitespaceSize);
  initIntSpinBox(this->spinBoxTabWidth, Settings::Settings::tabWidth);

  initComboBox(this->comboBoxOctoPrintFileFormat, Settings::Settings::octoPrintFileFormat);
  initComboBox(this->comboBoxOctoPrintAction, Settings::Settings::octoPrintAction);
  initComboBox(this->comboBoxLocalAppFileFormat, Settings::Settings::localAppFileFormat);
  initComboBox(this->comboBoxRenderBackend3D, Settings::Settings::renderBackend3D);
  initComboBox(this->comboBoxToolbarExport3D, Settings::Settings::toolbarExport3D);
  initComboBox(this->comboBoxToolbarExport2D, Settings::Settings::toolbarExport2D);

  initListBox(this->listWidgetLocalAppParams, Settings::Settings::localAppParameterList);
  connect(this->listWidgetLocalAppParams->model(), &QAbstractItemModel::dataChanged, this,
          &Preferences::listWidgetLocalAppParamsModelDataChanged);
  connect(this->listWidgetLocalAppParams->model(), &QAbstractItemModel::rowsInserted, this,
          &Preferences::listWidgetLocalAppParamsModelDataChanged);
  connect(this->listWidgetLocalAppParams->model(), &QAbstractItemModel::rowsRemoved, this,
          &Preferences::listWidgetLocalAppParamsModelDataChanged);

  installIgnoreWheelWhenNotFocused(this);

  const QString slicer = QString::fromStdString(Settings::Settings::octoPrintSlicerEngine.value());
  const QString slicerDesc =
    QString::fromStdString(Settings::Settings::octoPrintSlicerEngineDesc.value());
  const QString profile = QString::fromStdString(Settings::Settings::octoPrintSlicerProfile.value());
  const QString profileDesc =
    QString::fromStdString(Settings::Settings::octoPrintSlicerProfileDesc.value());
  BlockSignals<QLineEdit *>(this->lineEditLocalAppExecutable)
    ->setText(QString::fromStdString(Settings::Settings::localAppExecutable.value()));
  BlockSignals<QLineEdit *>(this->lineEditLocalAppTempDir)
    ->setText(QString::fromStdString(Settings::Settings::localAppTempDir.value()));
  this->comboBoxOctoPrintSlicingEngine->clear();
  this->comboBoxOctoPrintSlicingEngine->addItem(_("<Default>"), QVariant{""});
  if (!slicer.isEmpty()) {
    this->comboBoxOctoPrintSlicingEngine->addItem(slicerDesc, QVariant{slicer});
  }
  this->comboBoxOctoPrintSlicingProfile->clear();
  this->comboBoxOctoPrintSlicingProfile->addItem(_("<Default>"), QVariant{""});
  if (!profile.isEmpty()) {
    this->comboBoxOctoPrintSlicingProfile->addItem(profileDesc, QVariant{profile});
  }

  emit editorConfigChanged();
}

Preferences::~Preferences() { removeDefaultSettings(); }

void Preferences::update()
{
  this->checkBoxAlwaysShowExportPdfDialog->setChecked(
    Settings::SettingsExportPdf::exportPdfAlwaysShowDialog.value());
  this->checkBoxAlwaysShowExport3mfDialog->setChecked(
    Settings::SettingsExport3mf::export3mfAlwaysShowDialog.value());
  this->checkBoxAlwaysShowPrintServiceDialog->setChecked(
    Settings::Settings::printServiceAlwaysShowDialog.value());
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
void Preferences::addPrefPage(QActionGroup *group, QAction *action, QWidget *widget)
{
  group->addAction(action);
  prefPages[action] = widget;
}

/**
 * Callback to switch pages in the preferences GUI.
 *
 * @param action The action triggered by the user.
 */
void Preferences::actionTriggered(QAction *action)
{
  this->stackedWidget->setCurrentWidget(prefPages[action]);
}

/**
 * Called at least on showing / closing the Preferences dialog
 * and when switching tabs.
 */
void Preferences::hidePasswords()
{
  this->pushButtonOctoPrintApiKey->setChecked(false);
  this->lineEditOctoPrintApiKey->setEchoMode(QLineEdit::EchoMode::PasswordEchoOnEdit);
}

void Preferences::on_stackedWidget_currentChanged(int)
{
  hidePasswords();
  this->labelOctoPrintCheckConnection->setText("");
  this->AxisConfig->updateStates();
  this->ButtonConfig->updateStates();
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
  auto *feature = v.value<Feature *>();
  feature->enable(state);
  QSettingsCached settings;
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
void Preferences::setupFeaturesPage()
{
  int row = 0;
  for (auto it = Feature::begin(); it != Feature::end(); ++it) {
    Feature *feature = *it;

    QString featurekey = QString("feature/%1").arg(QString::fromStdString(feature->get_name()));
    this->defaultmap[featurekey] = false;

    // spacer item between the features, just for some optical separation
    gridLayoutExperimentalFeatures->addItem(
      new QSpacerItem(1, 8, QSizePolicy::Expanding, QSizePolicy::Fixed), row, 1, 1, 1, Qt::AlignCenter);
    row++;

    auto *cb = new QCheckBox(QString::fromStdString(feature->get_name()), pageFeatures);
    QFont bold_font(cb->font());
    bold_font.setBold(true);
    cb->setFont(bold_font);
    // synchronize Qt settings with the feature settings
    bool value = getValue(featurekey).toBool();
    feature->enable(value);
    cb->setChecked(value);
    cb->setProperty(featurePropertyName, QVariant::fromValue<Feature *>(feature));
    connect(cb, &QCheckBox::toggled, this, &Preferences::featuresCheckBoxToggled);
    gridLayoutExperimentalFeatures->addWidget(cb, row, 0, 1, 2, Qt::AlignLeading);
    row++;

    auto *l = new QLabel(QString::fromStdString(feature->get_description()), pageFeatures);
    l->setTextFormat(Qt::RichText);
    gridLayoutExperimentalFeatures->addWidget(l, row, 1, 1, 1, Qt::AlignLeading);
    row++;
  }
  // Force fixed indentation, the checkboxes use column span of 2 so
  // first row is not constrained in size by the visible controls. The
  // fixed size space essentially gives the first row the width of the
  // spacer item itself.
  gridLayoutExperimentalFeatures->addItem(new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Fixed),
                                          1, 0, 1, 1, Qt::AlignLeading);
}

void Preferences::setup3DPrintPage()
{
  const auto& currentPrintService = Settings::Settings::defaultPrintService.value();
  const auto currentPrintServiceName =
    QString::fromStdString(Settings::Settings::printServiceName.value());
  checkBoxEnableRemotePrintServices->setChecked(Settings::Settings::enableRemotePrintServices.value());
  comboBoxDefaultPrintService->clear();
  const std::unordered_map<std::string, QString> services = {
    {"NONE", _("NONE")},
    {"OCTOPRINT", _("OctoPrint")},
    {"LOCAL_APPLICATION", _("Local Application")},
  };

  comboBoxDefaultPrintService->addItem(services.at("NONE"), QStringList{"NONE", ""});
  for (const auto& printServiceItem : PrintService::getPrintServices()) {
    const auto& key = printServiceItem.first;
    const auto& printService = printServiceItem.second;
    const auto settingValue = QStringList{"PRINT_SERVICE", QString::fromStdString(key)};
    const auto displayName = QString(printService->getDisplayName());
    comboBoxDefaultPrintService->addItem(displayName, settingValue);
    if (key == currentPrintServiceName.toStdString()) {
      comboBoxDefaultPrintService->setCurrentText(QString(printService->getDisplayName()));
    }
  }
  comboBoxDefaultPrintService->addItem(services.at("OCTOPRINT"), QStringList{"OCTOPRINT", ""});
  comboBoxDefaultPrintService->addItem(services.at("LOCAL_APPLICATION"),
                                       QStringList{"LOCAL_APPLICATION", ""});

  auto it = services.find(currentPrintService);
  if (it != services.end()) {
    comboBoxDefaultPrintService->setCurrentText(it->second);
  }
}

void Preferences::on_colorSchemeChooser_itemSelectionChanged()
{
  QString scheme = this->colorSchemeChooser->currentItem()->text();
  QSettingsCached settings;
  settings.setValue("3dview/colorscheme", scheme);
  emit colorSchemeChanged(scheme);
}

void Preferences::on_fontChooser_currentFontChanged(const QFont& font)
{
  QSettingsCached settings;
  settings.setValue("editor/fontfamily", font.family());
  emit fontChanged(font.family(), getValue("editor/fontsize").toUInt());
}

void Preferences::on_fontSize_currentIndexChanged(int index)
{
  uint intsize = this->fontSize->itemText(index).toUInt();
  QSettingsCached settings;
  settings.setValue("editor/fontsize", intsize);
  emit fontChanged(getValue("editor/fontfamily").toString(), intsize);
}

void Preferences::on_syntaxHighlight_currentTextChanged(const QString& s)
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
  if (AutoUpdater *updater = AutoUpdater::updater()) {
    updater->setAutomaticallyChecksForUpdates(on);
  } else {
    unimplemented_msg();
  }
}

void Preferences::on_snapshotCheckBox_toggled(bool on)
{
  if (AutoUpdater *updater = AutoUpdater::updater()) {
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

void Preferences::on_reorderCheckBox_toggled(bool state)
{
  if (!state) {
    undockCheckBox->setChecked(false);
  }
  undockCheckBox->setEnabled(state);
  QSettingsCached settings;
  settings.setValue("advanced/reorderWindows", state);
  emit updateReorderMode(state);
}

void Preferences::on_undockCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/undockableWindows", state);
  emit updateUndockMode(state);
}

void Preferences::on_openCSGWarningBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/opencsg_show_warning", state);
}

void Preferences::on_cgalCacheSizeMBEdit_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("advanced/cgalCacheSizeMB", text);
  CGALCache::instance()->setMaxSizeMB(text.toULong());
}

void Preferences::on_polysetCacheSizeMBEdit_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("advanced/polysetCacheSizeMB", text);
  GeometryCache::instance()->setMaxSizeMB(text.toULong());
}

void Preferences::on_opencsgLimitEdit_textChanged(const QString& text)
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

void Preferences::on_autoReloadRaiseCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/autoReloadRaise", state);
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
  Settings::Settings::showWarningsIn3dView.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxMouseCentricZoom_toggled(bool val)
{
  Settings::Settings::mouseCentricZoom.setValue(val);
  writeSettings();
  emit updateMouseCentricZoom(val);
}

void Preferences::on_spinBoxIndentationWidth_valueChanged(int val)
{
  Settings::Settings::indentationWidth.setValue(val);
  writeSettings();
}

void Preferences::on_spinBoxTabWidth_valueChanged(int val)
{
  Settings::Settings::tabWidth.setValue(val);
  writeSettings();
}

void Preferences::on_comboBoxLineWrap_activated(int val)
{
  applyComboBox(comboBoxLineWrap, val, Settings::Settings::lineWrap);
}

void Preferences::on_comboBoxLineWrapIndentationStyle_activated(int val)
{
  // Next Line disables the Indent Spin-Box when 'Same' or 'Indented' is chosen from
  // LineWrapIndentationStyle Combo-Box.
  spinBoxLineWrapIndentationIndent->setDisabled(
    comboBoxLineWrapIndentationStyle->currentData() == "Same" ||
    comboBoxLineWrapIndentationStyle->currentData() == "Indented");

  applyComboBox(comboBoxLineWrapIndentationStyle, val, Settings::Settings::lineWrapIndentationStyle);
}

void Preferences::on_spinBoxLineWrapIndentationIndent_valueChanged(int val)
{
  Settings::Settings::lineWrapIndentation.setValue(val);
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
  Settings::Settings::showWhitespaceSize.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxAutoIndent_toggled(bool val)
{
  Settings::Settings::autoIndent.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxBackspaceUnindents_toggled(bool val)
{
  Settings::Settings::backspaceUnindents.setValue(val);
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
  Settings::Settings::highlightCurrentLine.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxEnableBraceMatching_toggled(bool val)
{
  Settings::Settings::enableBraceMatching.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxEnableLineNumbers_toggled(bool val)
{
  Settings::Settings::enableLineNumbers.setValue(val);
  writeSettings();
}

void Preferences::on_checkBoxEnableNumberScrollWheel_toggled(bool val)
{
  Settings::Settings::enableNumberScrollWheel.setValue(val);
  comboBoxModifierNumberScrollWheel->setDisabled(!val);
  writeSettings();
}

void Preferences::on_enableSoundOnRenderCompleteCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/enableSoundNotification", state);
  this->timeThresholdOnRenderCompleteSoundLabel->setEnabled(state);
  this->secLabelOnRenderCompleteSound->setEnabled(state);
  this->timeThresholdOnRenderCompleteSoundEdit->setEnabled(state);
}

void Preferences::on_timeThresholdOnRenderCompleteSoundEdit_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("advanced/timeThresholdOnRenderCompleteSound", text);
}

void Preferences::on_enableClearConsoleCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/consoleAutoClear", state);
}

void Preferences::on_consoleMaxLinesEdit_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("advanced/consoleMaxLines", text);
}

void Preferences::fireApplicationFontChanged() const
{
  const auto family = getValue("advanced/applicationFontFamily").toString();
  const auto size = getValue("advanced/applicationFontSize").toUInt();
  emit applicationFontChanged(family, size);
}

void Preferences::on_fontComboBoxApplicationFontFamily_currentFontChanged(const QFont& font)
{
  QSettingsCached settings;
  settings.setValue("advanced/applicationFontFamily", font.family());
  fireApplicationFontChanged();
}

void Preferences::on_comboBoxApplicationFontSize_currentIndexChanged(int index)
{
  uint intsize = this->comboBoxApplicationFontSize->itemText(index).toUInt();
  QSettingsCached settings;
  settings.setValue("advanced/applicationFontSize", intsize);
  fireApplicationFontChanged();
}

void Preferences::on_consoleFontChooser_currentFontChanged(const QFont& font)
{
  QSettingsCached settings;
  settings.setValue("advanced/consoleFontFamily", font.family());
  emit consoleFontChanged(font.family(), getValue("advanced/consoleFontSize").toUInt());
}

void Preferences::on_consoleFontSize_currentIndexChanged(int index)
{
  uint intsize = this->consoleFontSize->itemText(index).toUInt();
  QSettingsCached settings;
  settings.setValue("advanced/consoleFontSize", intsize);
  emit consoleFontChanged(getValue("advanced/consoleFontFamily").toString(), intsize);
}

void Preferences::on_customizerFontChooser_currentFontChanged(const QFont& font)
{
  QSettingsCached settings;
  settings.setValue("advanced/customizerFontFamily", font.family());
  emit customizerFontChanged(font.family(), getValue("advanced/customizerFontSize").toUInt());
}

void Preferences::on_customizerFontSize_currentIndexChanged(int index)
{
  uint intsize = this->customizerFontSize->itemText(index).toUInt();
  QSettingsCached settings;
  settings.setValue("advanced/customizerFontSize", intsize);
  emit customizerFontChanged(getValue("advanced/customizerFontFamily").toString(), intsize);
}

void Preferences::on_checkBoxEnableAutocomplete_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("editor/enableAutocomplete", state);
  this->labelCharacterThreshold->setEnabled(state);
  this->lineEditCharacterThreshold->setEnabled(state);
  emit autocompleteChanged(state);
}

void Preferences::on_lineEditCharacterThreshold_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("editor/characterThreshold", text);
  emit characterThresholdChanged(text.toInt());
}

void Preferences::on_lineEditStepSize_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("editor/stepSize", text);
  emit stepSizeChanged(text.toInt());
}

void Preferences::on_comboBoxModifierNumberScrollWheel_activated(int val)
{
  applyComboBox(comboBoxModifierNumberScrollWheel, val, Settings::Settings::modifierNumberScrollWheel);
}

void Preferences::on_enableHardwarningsCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/enableHardwarnings", state);
}

void Preferences::on_traceDepthEdit_textChanged(const QString& text)
{
  QSettingsCached settings;
  settings.setValue("advanced/traceDepth", text);
}

void Preferences::on_enableTraceUsermoduleParametersCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/enableTraceUsermoduleParameters", state);
}

void Preferences::on_enableParameterCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/enableParameterCheck", state);
}

void Preferences::on_enableRangeCheckBox_toggled(bool state)
{
  QSettingsCached settings;
  settings.setValue("advanced/enableParameterRangeCheck", state);
}

void Preferences::on_comboBoxRenderBackend3D_activated(int val)
{
  applyComboBox(this->comboBoxRenderBackend3D, val, Settings::Settings::renderBackend3D);
  RenderSettings::inst()->backend3D =
    renderBackend3DFromString(Settings::Settings::renderBackend3D.value())
      .value_or(DEFAULT_RENDERING_BACKEND_3D);
}

void Preferences::on_comboBoxToolbarExport3D_activated(int val)
{
  applyComboBox(this->comboBoxToolbarExport3D, val, Settings::Settings::toolbarExport3D);
  emit toolbarExportChanged();
}

void Preferences::on_comboBoxToolbarExport2D_activated(int val)
{
  applyComboBox(this->comboBoxToolbarExport2D, val, Settings::Settings::toolbarExport2D);
  emit toolbarExportChanged();
}

void Preferences::on_checkBoxSummaryCamera_toggled(bool checked)
{
  Settings::Settings::summaryCamera.setValue(checked);
  writeSettings();
}

void Preferences::on_checkBoxSummaryArea_toggled(bool checked)
{
  Settings::Settings::summaryArea.setValue(checked);
  writeSettings();
}

void Preferences::on_checkBoxSummaryBoundingBox_toggled(bool checked)
{
  Settings::Settings::summaryBoundingBox.setValue(checked);
  writeSettings();
}

void Preferences::on_enableHidapiTraceCheckBox_toggled(bool checked)
{
  Settings::Settings::inputEnableDriverHIDAPILog.setValue(checked);
  writeSettings();
}

void Preferences::on_checkBoxEnableRemotePrintServices_toggled(bool checked)
{
  S::enableRemotePrintServices.setValue(checked);
  writeSettings();
}

void Preferences::on_comboBoxDefaultPrintService_activated(int)
{
  QStringList currentPrintServiceList = comboBoxDefaultPrintService->currentData().toStringList();
  Settings::Settings::defaultPrintService.setValue(currentPrintServiceList[0].toStdString());
  Settings::Settings::printServiceName.setValue(currentPrintServiceList[1].toStdString());
  writeSettings();
}

void Preferences::on_comboBoxOctoPrintAction_activated(int val)
{
  applyComboBox(comboBoxOctoPrintAction, val, Settings::Settings::octoPrintAction);
}

void Preferences::on_lineEditOctoPrintURL_editingFinished()
{
  Settings::Settings::octoPrintUrl.setValue(this->lineEditOctoPrintURL->text().toStdString());
  writeSettings();
}

void Preferences::on_lineEditOctoPrintApiKey_editingFinished()
{
  Settings::Settings::octoPrintApiKey.setValue(this->lineEditOctoPrintApiKey->text().toStdString());
  writeSettings();
}

void Preferences::on_pushButtonOctoPrintApiKey_clicked()
{
  this->lineEditOctoPrintApiKey->setEchoMode(this->pushButtonOctoPrintApiKey->isChecked()
                                               ? QLineEdit::EchoMode::Normal
                                               : QLineEdit::EchoMode::PasswordEchoOnEdit);
}

void Preferences::on_pushButtonOctoPrintRequestApiKey_clicked()
{
  OctoPrintApiKeyDialog dialog;
  if (dialog.exec() == QDialog::Accepted) {
    const auto& apiKey = dialog.getApiKey();
    this->lineEditOctoPrintApiKey->setText(apiKey);
    S::octoPrintApiKey.setValue(apiKey.toStdString());
    writeSettings();
  }
}

void Preferences::on_comboBoxOctoPrintFileFormat_activated(int val)
{
  applyComboBox(this->comboBoxOctoPrintFileFormat, val, Settings::Settings::octoPrintFileFormat);
}

void Preferences::on_comboBoxLocalAppFileFormat_activated(int val)
{
  applyComboBox(this->comboBoxLocalAppFileFormat, val, Settings::Settings::localAppFileFormat);
  writeSettings();
}

void Preferences::on_lineEditLocalAppExecutable_editingFinished()
{
  Settings::Settings::localAppExecutable.setValue(
    this->lineEditLocalAppExecutable->text().toStdString());
  writeSettings();
}

void Preferences::on_toolButtonLocalAppSelectExecutable_clicked()
{
  const QString fileName = QFileDialog::getOpenFileName(this, "Select application");
  if (fileName.isEmpty()) {
    return;
  }

  this->lineEditLocalAppExecutable->setText(fileName);
  on_lineEditLocalAppExecutable_editingFinished();
}

void Preferences::on_lineEditLocalAppTempDir_editingFinished()
{
  Settings::Settings::localAppTempDir.setValue(this->lineEditLocalAppTempDir->text().toStdString());
  writeSettings();
}

void Preferences::on_toolButtonLocalAppSelectTempDir_clicked()
{
  const QString tempDir = QFileDialog::getExistingDirectory(this, "Select temporary directory");
  if (tempDir.isEmpty()) {
    return;
  }

  this->lineEditLocalAppTempDir->setText(tempDir);
  on_lineEditLocalAppTempDir_editingFinished();
}

void Preferences::moveListBoxRow(QListWidget *listBox, int offset)
{
  const auto& index = listBox->selectionModel()->currentIndex();
  int newRow = index.row() + offset;
  if (newRow >= 0 && newRow <= listBox->count()) {
    auto item = listBox->takeItem(index.row());
    listBox->insertItem(newRow, item);
    listBox->setCurrentRow(newRow);
  }
}

void Preferences::on_toolButtonLocalAppParameterUp_clicked()
{
  moveListBoxRow(this->listWidgetLocalAppParams, -1);
}

void Preferences::on_toolButtonLocalAppParameterDown_clicked()
{
  moveListBoxRow(this->listWidgetLocalAppParams, 1);
}

void Preferences::on_toolButtonLocalAppParameterRemove_clicked()
{
  const auto& index = this->listWidgetLocalAppParams->selectionModel()->currentIndex();
  if (index.row() >= 0) {
    auto item = this->listWidgetLocalAppParams->takeItem(index.row());
    delete item;
  }
}

void Preferences::insertListItem(QListWidget *listBox, QListWidgetItem *listItem)
{
  const auto hasSelection = listBox->selectionModel()->hasSelection();
  const auto pos = hasSelection ? listBox->selectionModel()->currentIndex().row() + 1 : listBox->count();
  listBox->insertItem(pos, listItem);
  listBox->setCurrentRow(pos);
  listBox->editItem(listItem);
}

void Preferences::on_toolButtonLocalAppParameterAdd_clicked()
{
  auto listItem =
    createListItem(Settings::LocalAppParameterType(Settings::LocalAppParameterType::string), "", true);
  insertListItem(this->listWidgetLocalAppParams, listItem);
}

void Preferences::addLocalAppParameter(const Settings::LocalAppParameterType& type)
{
  auto listItem = createListItem(Settings::LocalAppParameterType(type));
  insertListItem(this->listWidgetLocalAppParams, listItem);
}

void Preferences::on_toolButtonLocalAppParameterAddFile_clicked()
{
  addLocalAppParameter(Settings::LocalAppParameterType::file);
}

void Preferences::on_listWidgetLocalAppParams_itemSelectionChanged()
{
  const auto hasSelection = this->listWidgetLocalAppParams->selectionModel()->hasSelection();
  const auto& index = this->listWidgetLocalAppParams->selectionModel()->currentIndex();
  this->toolButtonLocalAppParameterRemove->setEnabled(hasSelection);
  this->toolButtonLocalAppParameterUp->setEnabled(hasSelection && index.row() > 0);
  this->toolButtonLocalAppParameterDown->setEnabled(
    hasSelection && index.row() < this->listWidgetLocalAppParams->count() - 1);
}

void Preferences::updateLocalAppParams()
{
  std::vector<Settings::LocalAppParameter> items;
  for (int idx = 0; idx < this->listWidgetLocalAppParams->count(); ++idx) {
    const auto item = this->listWidgetLocalAppParams->item(idx);
    if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                          static_cast<int>(Settings::LocalAppParameterType::string)) {
      items.emplace_back(Settings::LocalAppParameterType::string, item->text().toStdString());
    } else if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                                 static_cast<int>(Settings::LocalAppParameterType::file)) {
      items.emplace_back(Settings::LocalAppParameterType::file, std::string{});
    } else if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                                 static_cast<int>(Settings::LocalAppParameterType::dir)) {
      items.emplace_back(Settings::LocalAppParameterType::dir, std::string{});
    } else if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                                 static_cast<int>(Settings::LocalAppParameterType::extension)) {
      items.emplace_back(Settings::LocalAppParameterType::extension, std::string{});
    } else if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                                 static_cast<int>(Settings::LocalAppParameterType::source)) {
      items.emplace_back(Settings::LocalAppParameterType::source, std::string{});
    } else if (item->type() == static_cast<int>(QListWidgetItem::UserType) +
                                 static_cast<int>(Settings::LocalAppParameterType::sourcedir)) {
      items.emplace_back(Settings::LocalAppParameterType::sourcedir, std::string{});
    }
  }
  Settings::Settings::localAppParameterList.setValue(items);
  writeSettings();
}

void Preferences::on_listWidgetLocalAppParams_itemChanged(QListWidgetItem *)
{
  // called when parameter values are edited
  updateLocalAppParams();
}

void Preferences::listWidgetLocalAppParamsModelDataChanged()
{
  // called when rows are added or removed
  updateLocalAppParams();
}

void Preferences::on_actionLocalAppParameterFile_triggered()
{
  addLocalAppParameter(Settings::LocalAppParameterType::file);
}

void Preferences::on_actionLocalAppParameterDir_triggered()
{
  addLocalAppParameter(Settings::LocalAppParameterType::dir);
}

void Preferences::on_actionLocalAppParameterExtension_triggered()
{
  addLocalAppParameter(Settings::LocalAppParameterType::extension);
}

void Preferences::on_actionLocalAppParameterSource_triggered()
{
  addLocalAppParameter(Settings::LocalAppParameterType::source);
}

void Preferences::on_actionLocalAppParameterSourceDir_triggered()
{
  addLocalAppParameter(Settings::LocalAppParameterType::sourcedir);
}

void Preferences::on_pushButtonOctoPrintCheckConnection_clicked()
{
  OctoPrint octoPrint;

  try {
    QString api_version;
    QString server_version;
    std::tie(api_version, server_version) = octoPrint.getVersion();
    this->labelOctoPrintCheckConnection->setText(
      QString{_("Success: Server Version = %2, API Version = %1")}.arg(api_version).arg(server_version));
  } catch (const NetworkException& e) {
    QMessageBox::critical(this, _("Error"), QString::fromStdString(e.getErrorMessage()),
                          QMessageBox::Ok);
    this->labelOctoPrintCheckConnection->setText("");
  }
}

void Preferences::on_pushButtonOctoPrintSlicingEngine_clicked()
{
  OctoPrint octoPrint;

  const QString selection = this->comboBoxOctoPrintSlicingEngine->currentText();

  try {
    const auto slicers = octoPrint.getSlicers();
    this->comboBoxOctoPrintSlicingEngine->clear();
    this->comboBoxOctoPrintSlicingEngine->addItem(_("<Default>"), QVariant{""});
    for (const auto& entry : slicers) {
      this->comboBoxOctoPrintSlicingEngine->addItem(entry.second, QVariant{entry.first});
    }

    const int idx = this->comboBoxOctoPrintSlicingEngine->findText(selection);
    if (idx >= 0) {
      this->comboBoxOctoPrintSlicingEngine->setCurrentIndex(idx);
    }
  } catch (const NetworkException& e) {
    QMessageBox::critical(this, _("Error"), QString::fromStdString(e.getErrorMessage()),
                          QMessageBox::Ok);
  }
}

void Preferences::on_comboBoxOctoPrintSlicingEngine_activated(int val)
{
  const QString text = this->comboBoxOctoPrintSlicingEngine->itemData(val).toString();
  const QString desc = text.isEmpty() ? QString{} : this->comboBoxOctoPrintSlicingEngine->itemText(val);
  Settings::Settings::octoPrintSlicerEngine.setValue(text.toStdString());
  Settings::Settings::octoPrintSlicerEngineDesc.setValue(desc.toStdString());
  Settings::Settings::octoPrintSlicerProfile.setValue("");
  Settings::Settings::octoPrintSlicerProfileDesc.setValue("");
  writeSettings();
  this->comboBoxOctoPrintSlicingProfile->setCurrentIndex(0);
}

void Preferences::on_pushButtonOctoPrintSlicingProfile_clicked()
{
  OctoPrint octoPrint;

  const QString selection = this->comboBoxOctoPrintSlicingProfile->currentText();
  const QString slicer =
    this->comboBoxOctoPrintSlicingEngine->itemData(this->comboBoxOctoPrintSlicingEngine->currentIndex())
      .toString();

  try {
    const auto profiles = octoPrint.getProfiles(slicer);
    this->comboBoxOctoPrintSlicingProfile->clear();
    this->comboBoxOctoPrintSlicingProfile->addItem(_("<Default>"), QVariant{""});
    for (const auto& entry : profiles) {
      this->comboBoxOctoPrintSlicingProfile->addItem(entry.second, QVariant{entry.first});
    }

    const int idx = this->comboBoxOctoPrintSlicingProfile->findText(selection);
    if (idx >= 0) {
      this->comboBoxOctoPrintSlicingProfile->setCurrentIndex(idx);
    }
  } catch (const NetworkException& e) {
    QMessageBox::critical(this, _("Error"), QString::fromStdString(e.getErrorMessage()),
                          QMessageBox::Ok);
  }
}

void Preferences::on_comboBoxOctoPrintSlicingProfile_activated(int val)
{
  const QString text = this->comboBoxOctoPrintSlicingProfile->itemData(val).toString();
  const QString desc = text.isEmpty() ? QString{} : this->comboBoxOctoPrintSlicingProfile->itemText(val);
  Settings::Settings::octoPrintSlicerProfile.setValue(text.toStdString());
  Settings::Settings::octoPrintSlicerProfileDesc.setValue(desc.toStdString());
  writeSettings();
}

void Preferences::on_checkBoxAlwaysShowExportPdfDialog_toggled(bool state)
{
  Settings::SettingsExportPdf::exportPdfAlwaysShowDialog.setValue(state);
  writeSettings();
}

void Preferences::on_checkBoxAlwaysShowExport3mfDialog_toggled(bool state)
{
  Settings::SettingsExport3mf::export3mfAlwaysShowDialog.setValue(state);
  writeSettings();
}

void Preferences::on_checkBoxAlwaysShowPrintServiceDialog_toggled(bool state)
{
  Settings::Settings::printServiceAlwaysShowDialog.setValue(state);
  writeSettings();
}

void Preferences::writeSettings()
{
  Settings::Settings::visit(SettingsWriter());
  fireEditorConfigChanged();
}

void Preferences::fireEditorConfigChanged() const { emit editorConfigChanged(); }

void Preferences::keyPressEvent(QKeyEvent *e)
{
#ifdef Q_OS_MACOS
  if (e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
    close();
  } else
#endif
    if ((e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_W) || e->key() == Qt::Key_Escape) {
    close();
  }
}

void Preferences::showEvent(QShowEvent *e)
{
  QMainWindow::showEvent(e);
  hidePasswords();
}

void Preferences::closeEvent(QCloseEvent *e)
{
  hidePasswords();
  QMainWindow::closeEvent(e);
}

/*!
   Removes settings that are the same as the default settings to avoid
   overwriting future changes to default settings.
 */
void Preferences::removeDefaultSettings()
{
  QSettingsCached settings;
  for (QSettings::SettingsMap::const_iterator iter = this->defaultmap.begin();
       iter != this->defaultmap.end(); iter++) {
    if (settings.value(iter.key()) == iter.value()) {
      settings.remove(iter.key());
    }
  }
}

QVariant Preferences::getValue(const QString& key) const
{
  QSettingsCached settings;
  assert(settings.contains(key) || this->defaultmap.contains(key));
  return settings.value(key, this->defaultmap[key]);
}

void Preferences::updateGUI()
{
  const auto found =
    this->colorSchemeChooser->findItems(getValue("3dview/colorscheme").toString(), Qt::MatchExactly);
  if (!found.isEmpty())
    BlockSignals<QListWidget *>(this->colorSchemeChooser)->setCurrentItem(found.first());

  updateGUIFontFamily(fontChooser, "editor/fontfamily");
  updateGUIFontSize(fontSize, "editor/fontsize");

  const auto shighlight = getValue("editor/syntaxhighlight").toString();
  const auto shidx = this->syntaxHighlight->findText(shighlight);
  const auto sheffidx = shidx >= 0 ? shidx : this->syntaxHighlight->findText("Off");
  if (sheffidx >= 0) {
    BlockSignals<QComboBox *>(this->syntaxHighlight)->setCurrentIndex(sheffidx);
  }

  BlockSignals<QCheckBox *>(this->mouseWheelZoomBox)
    ->setChecked(getValue("editor/ctrlmousewheelzoom").toBool());

  if (AutoUpdater *updater = AutoUpdater::updater()) {
    BlockSignals<QCheckBox *>(this->updateCheckBox)
      ->setChecked(updater->automaticallyChecksForUpdates());
    BlockSignals<QCheckBox *>(this->snapshotCheckBox)->setChecked(updater->enableSnapshots());
    BlockSignals<QLabel *>(this->lastCheckedLabel)->setText(updater->lastUpdateCheckDate());
  }

  BlockSignals<QCheckBox *>(this->openCSGWarningBox)
    ->setChecked(getValue("advanced/opencsg_show_warning").toBool());
  BlockSignals<QLineEdit *>(this->cgalCacheSizeMBEdit)
    ->setText(getValue("advanced/cgalCacheSizeMB").toString());
  BlockSignals<QLineEdit *>(this->polysetCacheSizeMBEdit)
    ->setText(getValue("advanced/polysetCacheSizeMB").toString());
  BlockSignals<QLineEdit *>(this->opencsgLimitEdit)
    ->setText(getValue("advanced/openCSGLimit").toString());
  BlockSignals<QCheckBox *>(this->localizationCheckBox)
    ->setChecked(getValue("advanced/localization").toBool());
  BlockSignals<QCheckBox *>(this->autoReloadRaiseCheckBox)
    ->setChecked(getValue("advanced/autoReloadRaise").toBool());
  BlockSignals<QCheckBox *>(this->forceGoldfeatherBox)
    ->setChecked(getValue("advanced/forceGoldfeather").toBool());
  BlockSignals<QCheckBox *>(this->reorderCheckBox)
    ->setChecked(getValue("advanced/reorderWindows").toBool());
  BlockSignals<QCheckBox *>(this->undockCheckBox)
    ->setChecked(getValue("advanced/undockableWindows").toBool());
  BlockSignals<QCheckBox *>(this->launcherBox)->setChecked(getValue("launcher/showOnStartup").toBool());
  BlockSignals<QCheckBox *>(this->enableSoundOnRenderCompleteCheckBox)
    ->setChecked(getValue("advanced/enableSoundNotification").toBool());
  BlockSignals<QLineEdit *>(this->timeThresholdOnRenderCompleteSoundEdit)
    ->setText(getValue("advanced/timeThresholdOnRenderCompleteSound").toString());
  BlockSignals<QCheckBox *>(this->enableClearConsoleCheckBox)
    ->setChecked(getValue("advanced/consoleAutoClear").toBool());
  BlockSignals<QLineEdit *>(this->consoleMaxLinesEdit)
    ->setText(getValue("advanced/consoleMaxLines").toString());

  updateGUIFontFamily(fontComboBoxApplicationFontFamily, "advanced/applicationFontFamily");
  updateGUIFontSize(comboBoxApplicationFontSize, "advanced/applicationFontSize");

  updateGUIFontFamily(consoleFontChooser, "advanced/consoleFontFamily");
  updateGUIFontSize(consoleFontSize, "advanced/consoleFontSize");

  updateGUIFontFamily(customizerFontChooser, "advanced/customizerFontFamily");
  updateGUIFontSize(customizerFontSize, "advanced/customizerFontSize");

  BlockSignals<QCheckBox *>(this->enableHardwarningsCheckBox)
    ->setChecked(getValue("advanced/enableHardwarnings").toBool());
  BlockSignals<QLineEdit *>(this->traceDepthEdit)->setText(getValue("advanced/traceDepth").toString());
  BlockSignals<QCheckBox *>(this->enableTraceUsermoduleParametersCheckBox)
    ->setChecked(getValue("advanced/enableTraceUsermoduleParameters").toBool());
  BlockSignals<QCheckBox *>(this->enableParameterCheckBox)
    ->setChecked(getValue("advanced/enableParameterCheck").toBool());
  BlockSignals<QCheckBox *>(this->enableRangeCheckBox)
    ->setChecked(getValue("advanced/enableParameterRangeCheck").toBool());
  updateComboBox(this->comboBoxToolbarExport3D, Settings::Settings::toolbarExport3D);
  updateComboBox(this->comboBoxToolbarExport2D, Settings::Settings::toolbarExport2D);

  BlockSignals<QCheckBox *>(this->checkBoxSummaryCamera)
    ->setChecked(Settings::Settings::summaryCamera.value());
  BlockSignals<QCheckBox *>(this->checkBoxSummaryArea)
    ->setChecked(Settings::Settings::summaryArea.value());
  BlockSignals<QCheckBox *>(this->checkBoxSummaryBoundingBox)
    ->setChecked(Settings::Settings::summaryBoundingBox.value());

  BlockSignals<QCheckBox *>(this->enableHidapiTraceCheckBox)
    ->setChecked(Settings::Settings::inputEnableDriverHIDAPILog.value());
  BlockSignals<QCheckBox *>(this->checkBoxEnableAutocomplete)
    ->setChecked(getValue("editor/enableAutocomplete").toBool());
  BlockSignals<QLineEdit *>(this->lineEditCharacterThreshold)
    ->setText(getValue("editor/characterThreshold").toString());
  BlockSignals<QLineEdit *>(this->lineEditStepSize)->setText(getValue("editor/stepSize").toString());

  this->secLabelOnRenderCompleteSound->setEnabled(getValue("advanced/enableSoundNotification").toBool());
  this->undockCheckBox->setEnabled(this->reorderCheckBox->isChecked());
  this->timeThresholdOnRenderCompleteSoundLabel->setEnabled(
    getValue("advanced/enableSoundNotification").toBool());
  this->timeThresholdOnRenderCompleteSoundEdit->setEnabled(
    getValue("advanced/enableSoundNotification").toBool());
  this->labelCharacterThreshold->setEnabled(getValue("editor/enableAutocomplete").toBool());
  this->lineEditCharacterThreshold->setEnabled(getValue("editor/enableAutocomplete").toBool());
  this->lineEditStepSize->setEnabled(getValue("editor/stepSize").toBool());

  updateComboBox(this->comboBoxRenderBackend3D, Settings::Settings::renderBackend3D);
  updateComboBox(this->comboBoxLineWrap, Settings::Settings::lineWrap);
  updateComboBox(this->comboBoxLineWrapIndentationStyle, Settings::Settings::lineWrapIndentationStyle);
  updateComboBox(this->comboBoxLineWrapVisualizationStart,
                 Settings::Settings::lineWrapVisualizationBegin);
  updateComboBox(this->comboBoxLineWrapVisualizationEnd, Settings::Settings::lineWrapVisualizationEnd);
  updateComboBox(this->comboBoxShowWhitespace, Settings::Settings::showWhitespace);
  updateComboBox(this->comboBoxIndentUsing, Settings::Settings::indentStyle);
  updateComboBox(this->comboBoxTabKeyFunction, Settings::Settings::tabKeyFunction);
  updateComboBox(this->comboBoxModifierNumberScrollWheel, Settings::Settings::modifierNumberScrollWheel);
  updateIntSpinBox(this->spinBoxIndentationWidth, Settings::Settings::indentationWidth);
  updateIntSpinBox(this->spinBoxTabWidth, Settings::Settings::tabWidth);
  updateIntSpinBox(this->spinBoxLineWrapIndentationIndent, Settings::Settings::lineWrapIndentation);
  updateIntSpinBox(this->spinBoxShowWhitespaceSize, Settings::Settings::showWhitespaceSize);
  initUpdateCheckBox(this->checkBoxAutoIndent, Settings::Settings::autoIndent);
  initUpdateCheckBox(this->checkBoxBackspaceUnindents, Settings::Settings::backspaceUnindents);
  initUpdateCheckBox(this->checkBoxHighlightCurrentLine, Settings::Settings::highlightCurrentLine);
  initUpdateCheckBox(this->checkBoxEnableBraceMatching, Settings::Settings::enableBraceMatching);
  initUpdateCheckBox(this->checkBoxEnableNumberScrollWheel, Settings::Settings::enableNumberScrollWheel);
  initUpdateCheckBox(this->checkBoxShowWarningsIn3dView, Settings::Settings::showWarningsIn3dView);
  initUpdateCheckBox(this->checkBoxMouseCentricZoom, Settings::Settings::mouseCentricZoom);
  initUpdateCheckBox(this->checkBoxEnableLineNumbers, Settings::Settings::enableLineNumbers);

  /* Next Line disables the Indent Spin-Box,for 'Same' and 'Indented' LineWrapStyle selection from
     LineWrapIndentationStyle Combo-box, just after launching the openscad application. Removing this
     line will cause misbehaviour, and will not disable the Indent spin-box until you interact with the
     LineWrapStyle Combo-Box first-time and choose a style for which disabling has been handled. For
     normal cases, a similar line, inside the function 'on_comboBoxLineWrapIndentationStyle_activated()'
     handles the disabling functionality.
   */
  this->spinBoxLineWrapIndentationIndent->setDisabled(
    comboBoxLineWrapIndentationStyle->currentData() == "Same" ||
    comboBoxLineWrapIndentationStyle->currentData() == "Indented");
  this->comboBoxModifierNumberScrollWheel->setDisabled(!checkBoxEnableNumberScrollWheel->isChecked());
  BlockSignals<QLineEdit *>(this->lineEditOctoPrintURL)
    ->setText(QString::fromStdString(Settings::Settings::octoPrintUrl.value()));
  BlockSignals<QLineEdit *>(this->lineEditOctoPrintApiKey)
    ->setText(QString::fromStdString(Settings::Settings::octoPrintApiKey.value()));
  updateComboBox(this->comboBoxOctoPrintAction, Settings::Settings::octoPrintAction);
  updateComboBox(this->comboBoxOctoPrintSlicingEngine,
                 Settings::Settings::octoPrintSlicerEngine.value());
  updateComboBox(this->comboBoxOctoPrintSlicingProfile,
                 Settings::Settings::octoPrintSlicerProfile.value());
}

void Preferences::applyComboBox(QComboBox * /*comboBox*/, int val,
                                Settings::SettingsEntryEnum<std::string>& entry)
{
  entry.setIndex(val);
  writeSettings();
}

void Preferences::apply_win() const
{
  emit requestRedraw();
  emit openCSGSettingsChanged();
}

bool Preferences::hasHighlightingColorScheme() const
{
  return BlockSignals<QComboBox *>(syntaxHighlight)->count() != 0;
}

void Preferences::setHighlightingColorSchemes(const QStringList& colorSchemes)
{
  auto combobox = BlockSignals<QComboBox *>(syntaxHighlight);
  combobox->clear();
  combobox->addItems(colorSchemes);
}

void Preferences::createFontSizeMenu(QComboBox *boxarg, const QString& setting)
{
  uint savedsize = getValue(setting).toUInt();
  const QFontDatabase db;
  BlockSignals<QComboBox *> box{boxarg};
  for (auto size : db.standardSizes()) {
    box->addItem(QString::number(size));
    if (static_cast<uint>(size) == savedsize) {
      box->setCurrentIndex(box->count() - 1);
    }
  }
  // reset GUI fontsize if fontSize->addItem emitted signals that changed it.
  box->setEditText(QString("%1").arg(savedsize));
}

void Preferences::updateGUIFontFamily(QFontComboBox *ffSelector, const QString& setting)
{
  const auto fontfamily = getValue(setting).toString();
  const auto fidx = ffSelector->findText(fontfamily, Qt::MatchContains);
  if (fidx >= 0) {
    BlockSignals<QFontComboBox *>(ffSelector)->setCurrentIndex(fidx);
  }
}

void Preferences::updateGUIFontSize(QComboBox *fsSelector, const QString& setting)
{
  const auto fontsize = getValue(setting).toString();
  const auto sidx = fsSelector->findText(fontsize);
  if (sidx >= 0) {
    BlockSignals<QComboBox *>(fsSelector)->setCurrentIndex(sidx);
  } else {
    BlockSignals<QComboBox *>(fsSelector)->setEditText(fontsize);
  }
}

Preferences *GlobalPreferences::inst()
{
  static auto *instance = new Preferences();
  return instance;
};
