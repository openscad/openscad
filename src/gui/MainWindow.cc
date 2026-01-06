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

#include "gui/MainWindow.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>

#include <boost/version.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialog>
#include <QDockWidget>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QMutexLocker>
#include <QPoint>
#include <QProcess>
#include <QProgressDialog>
#include <QScreen>
#include <QSettings>  //Include QSettings for direct operations on settings arrays
#include <QSignalMapper>
#include <QSoundEffect>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "core/AST.h"
#include "core/BuiltinContext.h"
#include "core/Builtins.h"
#include "core/CSGNode.h"
#include "core/Context.h"
#include "core/customizer/CommentParser.h"
#include "core/EvaluationSession.h"
#include "core/Expression.h"
#include "core/node.h"
#include "core/parsersettings.h"
#include "core/progress.h"
#include "core/RenderVariables.h"
#include "core/ScopeContext.h"
#include "core/Settings.h"
#include "core/SourceFileCache.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryCache.h"
#include "geometry/GeometryEvaluator.h"
#include "glview/PolySetRenderer.h"
#include "glview/cgal/CGALRenderer.h"
#include "glview/preview/CSGTreeNormalizer.h"
#include "glview/preview/ThrownTogetherRenderer.h"
#include "glview/RenderSettings.h"
#include "gui/AboutDialog.h"
#include "gui/CGALWorker.h"
#include "gui/ColorList.h"
#include "gui/Editor.h"
#include "gui/Dock.h"
#include "gui/Measurement.h"
#include "gui/Export3mfDialog.h"
#include "gui/ExportPdfDialog.h"
#include "gui/ExportSvgDialog.h"
#include "gui/ExternalToolInterface.h"
#include "gui/ImportUtils.h"
#include "gui/input/InputDriverEvent.h"
#include "gui/input/InputDriverManager.h"
#include "gui/LibraryInfoDialog.h"
#include "gui/OpenSCADApp.h"
#include "gui/Preferences.h"
#include "gui/PrintInitDialog.h"
#include "gui/ProgressWidget.h"
#include "gui/QGLView.h"
#include "gui/QSettingsCached.h"
#include "gui/QWordSearchField.h"
#include "gui/SettingsWriter.h"
#include "gui/ScintillaEditor.h"
#include "gui/TabManager.h"
#include "gui/UIUtils.h"
#include "io/dxfdim.h"
#include "io/export.h"
#include "io/fileutils.h"
#include "openscad.h"
#include "platform/PlatformUtils.h"
#include "utils/exceptions.h"
#include "utils/printutils.h"
#include "version.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/CGALCache.h"
#include "geometry/cgal/CGALNefGeometry.h"
#endif  // ENABLE_CGAL
#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#include "geometry/manifold/ManifoldGeometry.h"
#endif  // ENABLE_MANIFOLD
#ifdef ENABLE_OPENCSG
#include "core/CSGTreeEvaluator.h"
#include "glview/preview/OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#ifdef OPENSCAD_UPDATER
#include "gui/AutoUpdater.h"
#endif

#ifdef ENABLE_PYTHON
#include "python/python_public.h"
#include "nettle/sha2.h"
#include "nettle/base64.h"

std::string SHA256HashString(std::string aString)
{
  uint8_t digest[SHA256_DIGEST_SIZE];
  sha256_ctx sha256_ctx;

  sha256_init(&sha256_ctx);
  sha256_update(&sha256_ctx, aString.length(), (uint8_t *)aString.c_str());
  sha256_digest(&sha256_ctx, SHA256_DIGEST_SIZE, digest);

  base64_encode_ctx base64_ctx;
  char digest_base64[BASE64_ENCODE_LENGTH(SHA256_DIGEST_SIZE) + 1];
  memset(digest_base64, 0, sizeof(digest_base64));

  base64_encode_init(&base64_ctx);
  base64_encode_update(&base64_ctx, digest_base64, SHA256_DIGEST_SIZE, digest);
  base64_encode_final(&base64_ctx, digest_base64);
  return digest_base64;
}

#endif  // ifdef ENABLE_PYTHON

#include "gui/PrintService.h"
#include "input/MouseConfigWidget.h"

// Global application state
unsigned int GuiLocker::guiLocked = 0;

bool MainWindow::undockMode = false;
bool MainWindow::reorderMode = false;
const int MainWindow::tabStopWidth = 15;
QElapsedTimer *MainWindow::progressThrottle = new QElapsedTimer();

namespace {

const int autoReloadPollingPeriodMS = 200;
const char copyrighttext[] =
  "<p>Copyright (C) 2009-2025 The OpenSCAD Developers</p>"
  "<p>This program is free software; you can redistribute it and/or modify "
  "it under the terms of the GNU General Public License as published by "
  "the Free Software Foundation; either version 2 of the License, or "
  "(at your option) any later version.<p>";

struct DockFocus {
  Dock *widget;
  std::function<void(MainWindow *)> focus;
};

QAction *findAction(const QList<QAction *>& actions, const std::string& name)
{
  for (const auto action : actions) {
    if (action->objectName().toStdString() == name) {
      return action;
    }
    if (action->menu()) {
      auto foundAction = findAction(action->menu()->actions(), name);
      if (foundAction) return foundAction;
    }
  }
  return nullptr;
}

void fileExportedMessage(const QString& format, const QString& filename)
{
  LOG("%1$s export finished: %2$s", format.toUtf8().constData(), filename.toUtf8().constData());
}

void removeExportActions(QToolBar *toolbar, QAction *action)
{
  int idx = toolbar->actions().indexOf(action);
  while (idx > 0) {
    QAction *a = toolbar->actions().at(idx - 1);
    if (a->objectName().isEmpty())  // separator
      break;
    toolbar->removeAction(a);
    idx--;
  }
}

std::unique_ptr<ExternalToolInterface> createExternalToolService(print_service_t serviceType,
                                                                 const QString& serviceName,
                                                                 FileFormat fileFormat)
{
  switch (serviceType) {
  case print_service_t::NONE:
    // TODO: Print warning
    return nullptr;
    break;
  case print_service_t::PRINT_SERVICE: {
    if (const auto printService = PrintService::getPrintService(serviceName.toStdString())) {
      return createExternalPrintService(printService, fileFormat);
    }
    LOG("Unknown print service \"%1$s\"", serviceName.toStdString());
    return nullptr;
    break;
  }
  case print_service_t::OCTOPRINT:         return createOctoPrintService(fileFormat); break;
  case print_service_t::LOCAL_APPLICATION: return createLocalProgramService(fileFormat); break;
  }
  return {};
}

}  // namespace

MainWindow::MainWindow(const QStringList& filenames) : rubberBandManager(this)
{
  installEventFilter(this);
  setupUi(this);

  consoleUpdater = new QTimer(this);
  consoleUpdater->setSingleShot(true);
  connect(consoleUpdater, &QTimer::timeout, this->console, &Console::update);

  this->animateWidget->setMainWindow(this);
  this->viewportControlWidget->setMainWindow(this);
  // actions not included in menu
  this->addAction(editActionInsertTemplate);
  this->addAction(editActionFoldAll);

  docks = {{editorDock, _("Editor"), "view/hideEditor"},
           {consoleDock, _("Console"), "view/hideConsole"},
           {parameterDock, _("Customizer"), "view/hideCustomizer"},
           {errorLogDock, _("Error-Log"), "view/hideErrorLog"},
           {animateDock, _("Animate"), "view/hideAnimate"},
           {fontListDock, _("Font List"), "view/hideFontList"},
           {colorListDock, _("Color List"), "view/hideColorList"},
           {viewportControlDock, _("Viewport-Control"), "view/hideViewportControl"}};

  this->versionLabel = nullptr;  // must be initialized before calling updateStatusBar()
  updateStatusBar(nullptr);

  renderCompleteSoundEffect = new QSoundEffect();
  renderCompleteSoundEffect->setSource(QUrl("qrc:/sounds/complete.wav"));

  absoluteRootNode = nullptr;

  // Open Recent
  for (auto& recent : this->actionRecentFile) {
    recent = new QAction(this);
    recent->setVisible(false);
    this->menuOpenRecent->addAction(recent);
    connect(recent, &QAction::triggered, this, &MainWindow::actionOpenRecent);
  }

  // Preferences initialization happens on first tab creation, and depends on colorschemes from editor.
  // Any code dependent on Preferences must come after the TabManager instantiation
  tabManager = new TabManager(this, filenames.isEmpty() ? QString() : filenames[0]);
  editorDockContents->layout()->addWidget(tabManager->getTabContent());

  connect(this, &MainWindow::highlightError, tabManager, &TabManager::highlightError);
  connect(this, &MainWindow::unhighlightLastError, tabManager, &TabManager::unhighlightLastError);

  connect(this->editActionUndo, &QAction::triggered, tabManager, &TabManager::undo);
  connect(this->editActionRedo, &QAction::triggered, tabManager, &TabManager::redo);
  connect(this->editActionRedo_2, &QAction::triggered, tabManager, &TabManager::redo);
  connect(this->editActionCut, &QAction::triggered, tabManager, &TabManager::cut);
  connect(this->editActionPaste, &QAction::triggered, tabManager, &TabManager::paste);

  connect(this->editActionIndent, &QAction::triggered, tabManager, &TabManager::indentSelection);
  connect(this->editActionUnindent, &QAction::triggered, tabManager, &TabManager::unindentSelection);
  connect(this->editActionComment, &QAction::triggered, tabManager, &TabManager::commentSelection);
  connect(this->editActionUncomment, &QAction::triggered, tabManager, &TabManager::uncommentSelection);

  connect(this->editActionToggleBookmark, &QAction::triggered, tabManager, &TabManager::toggleBookmark);
  connect(this->editActionNextBookmark, &QAction::triggered, tabManager, &TabManager::nextBookmark);
  connect(this->editActionPrevBookmark, &QAction::triggered, tabManager, &TabManager::prevBookmark);
  connect(this->editActionJumpToNextError, &QAction::triggered, tabManager,
          &TabManager::jumpToNextError);

  connect(tabManager, &TabManager::editorAboutToClose, this,
          &MainWindow::onTabManagerAboutToCloseEditor);
  connect(tabManager, &TabManager::currentEditorChanged, this, &MainWindow::onTabManagerEditorChanged);
  connect(tabManager, &TabManager::editorContentReloaded, this,
          &MainWindow::onTabManagerEditorContentReloaded);

  connect(this->console, &Console::openWindowRequested, this, &MainWindow::showLink);
  connect(GlobalPreferences::inst(), &Preferences::consoleFontChanged, this->console, &Console::setFont);
  this->console->setConsoleFont(
    GlobalPreferences::inst()->getValue("advanced/consoleFontFamily").toString(),
    GlobalPreferences::inst()->getValue("advanced/consoleFontSize").toUInt());

  const QString version =
    QString("<b>OpenSCAD %1</b>").arg(QString::fromStdString(std::string(openscad_versionnumber)));
  const QString weblink = "<a href=\"https://www.openscad.org/\">https://www.openscad.org/</a><br>";

  consoleOutputRaw(version);
  consoleOutputRaw(weblink);
  consoleOutputRaw(copyrighttext);
  this->consoleUpdater->start(0);  // Show "Loaded Design" message from TabManager

  connect(this->errorLogWidget, &ErrorLog::openFile, this, &MainWindow::openFileFromPath);
  connect(this->console, &Console::openFile, this, &MainWindow::openFileFromPath);

  connect(GlobalPreferences::inst()->ButtonConfig, &ButtonConfigWidget::inputMappingChanged,
          InputDriverManager::instance(), &InputDriverManager::onInputMappingUpdated,
          Qt::UniqueConnection);
  connect(GlobalPreferences::inst()->AxisConfig, &AxisConfigWidget::inputMappingChanged,
          InputDriverManager::instance(), &InputDriverManager::onInputMappingUpdated,
          Qt::UniqueConnection);
  connect(GlobalPreferences::inst()->AxisConfig, &AxisConfigWidget::inputCalibrationChanged,
          InputDriverManager::instance(), &InputDriverManager::onInputCalibrationUpdated,
          Qt::UniqueConnection);
  connect(GlobalPreferences::inst()->AxisConfig, &AxisConfigWidget::inputGainChanged,
          InputDriverManager::instance(), &InputDriverManager::onInputGainUpdated, Qt::UniqueConnection);

  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->setAttribute(Qt::WA_DeleteOnClose);

  scadApp->windowManager.add(this);

  this->cgalworker = new CGALWorker();
  connect(this->cgalworker, &CGALWorker::done, this, &MainWindow::actionRenderDone);

  rootNode = nullptr;

  this->qglview->statusLabel = new QLabel(this);
  this->qglview->statusLabel->setMinimumWidth(100);
  statusBar()->addWidget(this->qglview->statusLabel);

  const QSettingsCached settings;
  this->qglview->setMouseCentricZoom(Settings::Settings::mouseCentricZoom.value());
  this->setAllMouseViewActions();
  this->meas.setView(qglview);
  resetMeasurementsState(false, "Render (not preview) to enable measurements");

  autoReloadTimer = new QTimer(this);
  autoReloadTimer->setSingleShot(false);
  autoReloadTimer->setInterval(autoReloadPollingPeriodMS);
  connect(autoReloadTimer, &QTimer::timeout, this, &MainWindow::checkAutoReload);

  this->exportFormatMapper = new QSignalMapper(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  connect(this->exportFormatMapper, &QSignalMapper::mappedInt, this,
          &MainWindow::actionExportFileFormat);
#else
  connect(this->exportFormatMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
          this, &MainWindow::actionExportFileFormat);
#endif

  waitAfterReloadTimer = new QTimer(this);
  waitAfterReloadTimer->setSingleShot(true);
  waitAfterReloadTimer->setInterval(autoReloadPollingPeriodMS);
  connect(waitAfterReloadTimer, &QTimer::timeout, this, &MainWindow::waitAfterReload);
  connect(GlobalPreferences::inst(), &Preferences::ExperimentalChanged, this,
          &MainWindow::changeParameterWidget);

  progressThrottle->start();

  this->hideFind();
  frameCompileResult->hide();
  this->labelCompileResultMessage->setOpenExternalLinks(false);
  connect(this->labelCompileResultMessage, &QLabel::linkActivated, this, &MainWindow::showLink);

  // File menu
  connect(this->fileActionNewWindow, &QAction::triggered, this, &MainWindow::actionNewWindow);
  connect(this->fileActionNew, &QAction::triggered, tabManager, &TabManager::actionNew);
  connect(this->fileActionOpenWindow, &QAction::triggered, this, &MainWindow::actionOpenWindow);
  connect(this->fileActionOpen, &QAction::triggered, this, &MainWindow::actionOpen);
  connect(this->fileActionSave, &QAction::triggered, this, &MainWindow::actionSave);
  connect(this->fileActionSaveAs, &QAction::triggered, this, &MainWindow::actionSaveAs);
  connect(this->fileActionSaveACopy, &QAction::triggered, this, &MainWindow::actionSaveACopy);
  connect(this->fileActionSaveAll, &QAction::triggered, tabManager, &TabManager::saveAll);
  connect(this->fileActionReload, &QAction::triggered, this, &MainWindow::actionReload);
  connect(this->fileActionClose, &QAction::triggered, tabManager, &TabManager::closeCurrentTab);
  connect(this->fileActionQuit, &QAction::triggered, scadApp, &OpenSCADApp::quit, Qt::QueuedConnection);
  connect(this->fileShowLibraryFolder, &QAction::triggered, this, &MainWindow::actionShowLibraryFolder);

#ifdef ENABLE_PYTHON
  connect(this->fileActionPythonRevoke, &QAction::triggered, this,
          &MainWindow::actionPythonRevokeTrustedFiles);
  connect(this->fileActionPythonCreateVenv, &QAction::triggered, this,
          &MainWindow::actionPythonCreateVenv);
  connect(this->fileActionPythonSelectVenv, &QAction::triggered, this,
          &MainWindow::actionPythonSelectVenv);
#else
  this->menuPython->menuAction()->setVisible(false);
#endif

#ifndef __APPLE__
  auto shortcuts = this->fileActionSave->shortcuts();
  this->fileActionSave->setShortcuts(shortcuts);
  shortcuts = this->fileActionReload->shortcuts();
  shortcuts.push_back(QKeySequence(Qt::Key_F3));
  this->fileActionReload->setShortcuts(shortcuts);
#endif

  this->menuOpenRecent->addSeparator();
  this->menuOpenRecent->addAction(this->fileActionClearRecent);
  connect(this->fileActionClearRecent, &QAction::triggered, this, &MainWindow::clearRecentFiles);

  show_examples();

  connect(this->editActionNextTab, &QAction::triggered, tabManager, &TabManager::nextTab);
  connect(this->editActionPrevTab, &QAction::triggered, tabManager, &TabManager::prevTab);

  connect(this->editActionCopy, &QAction::triggered, this, &MainWindow::copyText);
  connect(this->editActionCopyViewport, &QAction::triggered, this, &MainWindow::actionCopyViewport);
  connect(this->editActionConvertTabsToSpaces, &QAction::triggered, this,
          &MainWindow::convertTabsToSpaces);
  connect(this->editActionCopyVPT, &QAction::triggered, this, &MainWindow::copyViewportTranslation);
  connect(this->editActionCopyVPR, &QAction::triggered, this, &MainWindow::copyViewportRotation);
  connect(this->editActionCopyVPD, &QAction::triggered, this, &MainWindow::copyViewportDistance);
  connect(this->editActionCopyVPF, &QAction::triggered, this, &MainWindow::copyViewportFov);
  connect(this->editActionPreferences, &QAction::triggered, this, &MainWindow::preferences);
  // Edit->Find
  connect(this->editActionFind, &QAction::triggered, this, &MainWindow::actionShowFind);
  connect(this->editActionFindAndReplace, &QAction::triggered, this,
          &MainWindow::actionShowFindAndReplace);
#ifdef Q_OS_WIN
  this->editActionFindAndReplace->setShortcut(QKeySequence("Ctrl+Shift+F"));
#endif
  connect(this->editActionFindNext, &QAction::triggered, this, &MainWindow::findNext);
  connect(this->editActionFindPrevious, &QAction::triggered, this, &MainWindow::findPrev);
  connect(this->editActionUseSelectionForFind, &QAction::triggered, this,
          &MainWindow::useSelectionForFind);

  // Design menu
  measurementGroup = new QActionGroup(this);
  measurementGroup->addAction(designActionMeasureDist);
  measurementGroup->addAction(designActionMeasureAngle);
  connect(this->designActionAutoReload, &QAction::toggled, this, &MainWindow::autoReloadSet);
  connect(this->designActionReloadAndPreview, &QAction::triggered, this,
          &MainWindow::actionReloadRenderPreview);
  connect(this->designActionPreview, &QAction::triggered, this, &MainWindow::actionRenderPreview);
  connect(this->designActionRender, &QAction::triggered, this, &MainWindow::actionRender);
  connect(this->measurementGroup, &QActionGroup::triggered, this, &MainWindow::handleMeasurementClicked);
  connect(this->designAction3DPrint, &QAction::triggered, this, &MainWindow::action3DPrint);
  connect(this->designCheckValidity, &QAction::triggered, this, &MainWindow::actionCheckValidity);
  connect(this->designActionDisplayAST, &QAction::triggered, this, &MainWindow::actionDisplayAST);
  connect(this->designActionDisplayCSGTree, &QAction::triggered, this,
          &MainWindow::actionDisplayCSGTree);
  connect(this->designActionDisplayCSGProducts, &QAction::triggered, this,
          &MainWindow::actionDisplayCSGProducts);

  exportMap[FileFormat::BINARY_STL] = this->fileActionExportBinarySTL;
  exportMap[FileFormat::ASCII_STL] = this->fileActionExportAsciiSTL;
  exportMap[FileFormat::_3MF] = this->fileActionExport3MF;
  exportMap[FileFormat::OBJ] = this->fileActionExportOBJ;
  exportMap[FileFormat::OFF] = this->fileActionExportOFF;
  exportMap[FileFormat::WRL] = this->fileActionExportWRL;
  exportMap[FileFormat::POV] = this->fileActionExportPOV;
  exportMap[FileFormat::AMF] = this->fileActionExportAMF;
  exportMap[FileFormat::DXF] = this->fileActionExportDXF;
  exportMap[FileFormat::SVG] = this->fileActionExportSVG;
  exportMap[FileFormat::PDF] = this->fileActionExportPDF;
  exportMap[FileFormat::CSG] = this->fileActionExportCSG;
  exportMap[FileFormat::PNG] = this->fileActionExportImage;

  for (auto& [format, action] : exportMap) {
    connect(action, &QAction::triggered, this->exportFormatMapper, QOverload<>::of(&QSignalMapper::map));
    this->exportFormatMapper->setMapping(action, int(format));
  }

  connect(this->designActionFlushCaches, &QAction::triggered, this, &MainWindow::actionFlushCaches);

#ifndef ENABLE_LIB3MF
  this->fileActionExport3MF->setVisible(false);
#endif

  // View menu
  this->viewActionThrownTogether->setEnabled(false);
  this->viewActionPreview->setEnabled(false);
  if (this->qglview->hasOpenCSGSupport()) {
    this->viewActionPreview->setChecked(true);
    this->viewActionThrownTogether->setChecked(false);
  } else {
    this->viewActionPreview->setChecked(false);
    this->viewActionThrownTogether->setChecked(true);
  }

  connect(this->viewActionPreview, &QAction::triggered, this, &MainWindow::viewModePreview);
  connect(this->viewActionThrownTogether, &QAction::triggered, this,
          &MainWindow::viewModeThrownTogether);
  connect(this->viewActionShowEdges, &QAction::triggered, this, &MainWindow::viewModeShowEdges);
  connect(this->viewActionShowAxes, &QAction::triggered, this, &MainWindow::viewModeShowAxes);
  connect(this->viewActionShowCrosshairs, &QAction::triggered, this,
          &MainWindow::viewModeShowCrosshairs);
  connect(this->viewActionShowScaleProportional, &QAction::triggered, this,
          &MainWindow::viewModeShowScaleProportional);
  connect(this->viewActionTop, &QAction::triggered, this, &MainWindow::viewAngleTop);
  connect(this->viewActionBottom, &QAction::triggered, this, &MainWindow::viewAngleBottom);
  connect(this->viewActionLeft, &QAction::triggered, this, &MainWindow::viewAngleLeft);
  connect(this->viewActionRight, &QAction::triggered, this, &MainWindow::viewAngleRight);
  connect(this->viewActionFront, &QAction::triggered, this, &MainWindow::viewAngleFront);
  connect(this->viewActionBack, &QAction::triggered, this, &MainWindow::viewAngleBack);
  connect(this->viewActionDiagonal, &QAction::triggered, this, &MainWindow::viewAngleDiagonal);
  connect(this->viewActionCenter, &QAction::triggered, this, &MainWindow::viewCenter);
  connect(this->viewActionResetView, &QAction::triggered, this, &MainWindow::viewResetView);
  connect(this->viewActionViewAll, &QAction::triggered, this, &MainWindow::viewAll);
  connect(this->viewActionPerspective, &QAction::triggered, this, &MainWindow::viewPerspective);
  connect(this->viewActionOrthogonal, &QAction::triggered, this, &MainWindow::viewOrthogonal);
  connect(this->viewActionZoomIn, &QAction::triggered, qglview, &QGLView::ZoomIn);
  connect(this->viewActionZoomOut, &QAction::triggered, qglview, &QGLView::ZoomOut);
  connect(this->viewActionHideEditorToolBar, &QAction::triggered, this, &MainWindow::hideEditorToolbar);
  connect(this->viewActionHide3DViewToolBar, &QAction::triggered, this, &MainWindow::hide3DViewToolbar);

  // Help menu
  connect(this->helpActionAbout, &QAction::triggered, this, &MainWindow::helpAbout);
  connect(this->helpActionHomepage, &QAction::triggered, this, &MainWindow::helpHomepage);
  connect(this->helpActionManual, &QAction::triggered, this, &MainWindow::helpManual);
  connect(this->helpActionCheatSheet, &QAction::triggered, this, &MainWindow::helpCheatSheet);
  connect(this->helpActionLibraryInfo, &QAction::triggered, this, &MainWindow::helpLibrary);

  // Checks if the Documentation has been downloaded and hides the Action otherwise
  if (UIUtils::hasOfflineUserManual()) {
    connect(this->helpActionOfflineManual, &QAction::triggered, this, &MainWindow::helpOfflineManual);
  } else {
    this->helpActionOfflineManual->setVisible(false);
  }
  if (UIUtils::hasOfflineCheatSheet()) {
    connect(this->helpActionOfflineCheatSheet, &QAction::triggered, this,
            &MainWindow::helpOfflineCheatSheet);
  } else {
    this->helpActionOfflineCheatSheet->setVisible(false);
  }
#ifdef OPENSCAD_UPDATER
  this->menuBar()->addMenu(AutoUpdater::updater()->updateMenu);
#endif

  connect(this->qglview, &QGLView::cameraChanged, animateWidget, &Animate::cameraChanged);
  connect(this->qglview, &QGLView::cameraChanged, viewportControlWidget,
          &ViewportControl::cameraChanged);
  connect(this->qglview, &QGLView::resized, viewportControlWidget, &ViewportControl::viewResized);
  connect(this->qglview, &QGLView::doRightClick, this, &MainWindow::rightClick);
  connect(this->qglview, &QGLView::doLeftClick, this, &MainWindow::leftClick);

  connect(GlobalPreferences::inst(), &Preferences::requestRedraw, this->qglview,
          QOverload<>::of(&QGLView::update));
  connect(GlobalPreferences::inst(), &Preferences::updateMouseCentricZoom, this->qglview,
          &QGLView::setMouseCentricZoom);
  connect(GlobalPreferences::inst()->MouseConfig, &MouseConfigWidget::updateMouseActions, this,
          &MainWindow::setAllMouseViewActions);
  connect(GlobalPreferences::inst(), &Preferences::updateReorderMode, this,
          &MainWindow::updateReorderMode);
  connect(GlobalPreferences::inst(), &Preferences::updateUndockMode, this,
          &MainWindow::updateUndockMode);
  connect(GlobalPreferences::inst(), &Preferences::openCSGSettingsChanged, this,
          &MainWindow::openCSGSettingsChanged);
  connect(GlobalPreferences::inst(), &Preferences::colorSchemeChanged, this,
          &MainWindow::setColorScheme);
  connect(GlobalPreferences::inst(), &Preferences::toolbarExportChanged, this,
          &MainWindow::updateExportActions);

  GlobalPreferences::inst()->apply_win();  // not sure if to be commented, checked must not be
                                           // commented(done some changes in apply())

  const QString cs = GlobalPreferences::inst()->getValue("3dview/colorscheme").toString();
  this->setColorScheme(cs);

  // find and replace panel
  connect(this->findTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::actionSelectFind);
  connect(this->findInputField, &QWordSearchField::textChanged, this, &MainWindow::findString);
  connect(this->findInputField, &QWordSearchField::returnPressed, this->findNextButton,
          [this] { this->findNextButton->animateClick(); });
  find_panel->installEventFilter(this);
  if (QApplication::clipboard()->supportsFindBuffer()) {
    connect(this->findInputField, &QWordSearchField::textChanged, this, &MainWindow::updateFindBuffer);
    connect(QApplication::clipboard(), &QClipboard::findBufferChanged, this,
            &MainWindow::findBufferChanged);
    // With Qt 4.8.6, there seems to be a bug that often gives an incorrect findbuffer content when
    // the app receives focus for the first time
    this->findInputField->setText(QApplication::clipboard()->text(QClipboard::FindBuffer));
  }

  connect(this->findPrevButton, &QPushButton::clicked, this, &MainWindow::findPrev);
  connect(this->findNextButton, &QPushButton::clicked, this, &MainWindow::findNext);
  connect(this->cancelButton, &QPushButton::clicked, this, &MainWindow::hideFind);
  connect(this->replaceButton, &QPushButton::clicked, this, &MainWindow::replace);
  connect(this->replaceAllButton, &QPushButton::clicked, this, &MainWindow::replaceAll);
  connect(this->replaceInputField, &QLineEdit::returnPressed, this->replaceButton,
          [this] { this->replaceButton->animateClick(); });
  addKeyboardShortCut(this->viewerToolBar->actions());
  addKeyboardShortCut(this->editortoolbar->actions());

  Preferences *instance = GlobalPreferences::inst();

  InputDriverManager::instance()->registerActions(this->menuBar()->actions(), "", "");
  InputDriverManager::instance()->registerActions(this->animateWidget->actions(), "animation",
                                                  "animate");
  instance->ButtonConfig->init();
  instance->MouseConfig->init();

  // fetch window states to be restored after restoreState() call
  const bool isEditorToolbarVisible = !settings.value("view/hideEditorToolbar").toBool();
  const bool is3DViewToolbarVisible = !settings.value("view/hide3DViewToolbar").toBool();

  // make sure it looks nice..
  const auto windowState = settings.value("window/state", QByteArray()).toByteArray();
  restoreGeometry(settings.value("window/geometry", QByteArray()).toByteArray());
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  // Workaround for a Qt bug (possible QTBUG-46620, but it's still there in Qt-6.5.3)
  // Blindly restoring a maximized window to a different screen resolution causes a crash
  // on the next move/resize operation on macOS:
  // https://github.com/openscad/openscad/issues/5486
  if (isMaximized()) {
    setGeometry(screen()->availableGeometry());
  }
#endif
  restoreState(windowState);

  if (windowState.size() == 0) {
    /*
     * This triggers only in case the configuration file has no
     * window state information (or no configuration file at all).
     * When this happens, the editor would default to a very ugly
     * width due to the dock widget layout. This overwrites the
     * value reported via sizeHint() to a width a bit smaller than
     * half the main window size (either the one loaded from the
     * configuration or the default value of 800).
     * The height is only a dummy value which will be essentially
     * ignored by the layouting as the editor is set to expand to
     * fill the available space.
     */
    activeEditor->setInitialSizeHint(QSize((5 * this->width() / 11), 100));
    tabifyDockWidget(consoleDock, errorLogDock);
    tabifyDockWidget(errorLogDock, fontListDock);
    tabifyDockWidget(fontListDock, colorListDock);
    tabifyDockWidget(colorListDock, animateDock);
    consoleDock->show();
  } else {
#ifdef Q_OS_WIN
    // Try moving the main window into the display range, this
    // can occur when closing OpenSCAD on a second monitor which
    // is not available at the time the application is started
    // again.
    // On Windows that causes the main window to open in a not
    // easily reachable place.
    auto primaryScreen = QApplication::primaryScreen();
    auto desktopRect = primaryScreen->availableGeometry().adjusted(250, 150, -250, -150).normalized();
    auto windowRect = frameGeometry();
    if (!desktopRect.intersects(windowRect)) {
      windowRect.moveCenter(desktopRect.center());
      windowRect = windowRect.intersected(desktopRect);
      move(windowRect.topLeft());
      resize(windowRect.size());
    }
#endif  // ifdef Q_OS_WIN
  }

  updateWindowSettings(isEditorToolbarVisible, is3DViewToolbarVisible);

  // Connect the menu "Windows/Navigation" to slot that process it by opening in a pop menu
  // the navigationMenu.
  connect(windowActionJumpTo, &QAction::triggered, this, &MainWindow::onNavigationOpenContextMenu);

  // Create the popup menu to navigate between the docks by keyboard.
  navigationMenu = new QMenu();

  // Create the docks, connect corresponding action and install menu entries
  for (auto& [dock, title, configKey] : docks) {
    dock->setName(title);
    dock->setConfigKey(configKey);
    dock->setVisible(!GlobalPreferences::inst()->getValue(configKey).toBool());
    dock->setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    // It is neede to have the event filter installed in each dock so that the events are
    // correctly processed when the dock are floating (is in a different window that the mainwindow)
    dock->installEventFilter(this);

    menuWindow->addAction(dock->toggleViewAction());

    auto dockAction = navigationMenu->addAction(title);
    dockAction->setProperty("id", QVariant::fromValue(dock));
    connect(dockAction, &QAction::triggered, this, &MainWindow::onNavigationTriggerContextMenuEntry);
    connect(dockAction, &QAction::hovered, this, &MainWindow::onNavigationHoveredContextMenuEntry);
  }

  connect(navigationMenu, &QMenu::aboutToHide, this, &MainWindow::onNavigationCloseContextMenu);
  connect(menuWindow, &QMenu::aboutToHide, this, &MainWindow::onNavigationCloseContextMenu);
  windowActionJumpTo->setMenu(navigationMenu);

  // connect the signal of next/prev windowAction and the dedicated slot
  // hovering is connected to rubberband activation while triggering is for actual
  // activation of the corresponding dock.
  const std::vector<QAction *> actions = {windowActionNextWindow, windowActionPreviousWindow};
  for (auto& action : actions) {
    connect(action, &QAction::hovered, this, &MainWindow::onWindowActionNextPrevHovered);
    connect(action, &QAction::triggered, this, &MainWindow::onWindowActionNextPrevTriggered);
  }

  // Adds shortcut for the prev/next window switching
  shortcutNextWindow = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
  QObject::connect(shortcutNextWindow, &QShortcut::activated, this,
                   &MainWindow::onWindowShortcutNextPrevActivated);
  shortcutPreviousWindow = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
  QObject::connect(shortcutPreviousWindow, &QShortcut::activated, this,
                   &MainWindow::onWindowShortcutNextPrevActivated);

  auto shortcutExport3D = new QShortcut(QKeySequence("F7"), this);
  QObject::connect(shortcutExport3D, &QShortcut::activated, this,
                   &MainWindow::onWindowShortcutExport3DActivated);

  // Adds dock specific behavior on visibility change
  QObject::connect(editorDock, &Dock::visibilityChanged, this,
                   &MainWindow::onEditorDockVisibilityChanged);
  QObject::connect(consoleDock, &Dock::visibilityChanged, this,
                   &MainWindow::onConsoleDockVisibilityChanged);
  QObject::connect(errorLogDock, &Dock::visibilityChanged, this,
                   &MainWindow::onErrorLogDockVisibilityChanged);
  QObject::connect(animateDock, &Dock::visibilityChanged, this,
                   &MainWindow::onAnimateDockVisibilityChanged);
  QObject::connect(fontListDock, &Dock::visibilityChanged, this,
                   &MainWindow::onFontListDockVisibilityChanged);
  QObject::connect(colorListDock, &Dock::visibilityChanged, this,
                   &MainWindow::onColorListDockVisibilityChanged);
  QObject::connect(viewportControlDock, &Dock::visibilityChanged, this,
                   &MainWindow::onViewportControlDockVisibilityChanged);
  QObject::connect(parameterDock, &Dock::visibilityChanged, this,
                   &MainWindow::onParametersDockVisibilityChanged);

  // Other dock specific signals
  QObject::connect(colorListWidget, &ColorList::colorSelected, this,
                   &MainWindow::onColorListColorSelected);

  connect(this->activeEditor, &EditorInterface::escapePressed, this, &MainWindow::measureFinished);
  // display this window and check for OpenGL 2.0 (OpenCSG) support
  viewModeThrownTogether();
  show();

  setCurrentOutput();

#ifdef ENABLE_OPENCSG
  viewModePreview();
#else
  viewModeThrownTogether();
#endif
  loadViewSettings();
  loadDesignSettings();

  setAcceptDrops(true);
  clearCurrentOutput();

  for (int i = 1; i < filenames.size(); ++i) tabManager->createTab(filenames[i]);

  updateExportActions();

  activeEditor->setFocus();

  // Configure the highlighting color scheme from the active editor one.
  // This is done only one time at creation of the first MainWindow instance
  auto preferences = GlobalPreferences::inst();
  if (!preferences->hasHighlightingColorScheme())
    preferences->setHighlightingColorSchemes(activeEditor->colorSchemes());

  onTabManagerEditorChanged(activeEditor);

  // fills the content of the Recents Files menu.
  updateRecentFileActions();
}

void MainWindow::setAllMouseViewActions()
{
  // Set the mouse actions to those held in the settings.
  this->qglview->setMouseActions(MouseConfig::MouseAction::LEFT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseLeftClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::MIDDLE_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseMiddleClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::RIGHT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseRightClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::SHIFT_LEFT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseShiftLeftClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::SHIFT_MIDDLE_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseShiftMiddleClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::SHIFT_RIGHT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseShiftRightClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_LEFT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlLeftClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_MIDDLE_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlMiddleClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_RIGHT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlRightClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_SHIFT_LEFT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlShiftLeftClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_SHIFT_MIDDLE_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlShiftMiddleClick.value())));
  this->qglview->setMouseActions(MouseConfig::MouseAction::CTRL_SHIFT_RIGHT_CLICK,
                                 MouseConfig::viewActionArrays.at(static_cast<MouseConfig::ViewAction>(
                                   Settings::Settings::inputMouseCtrlShiftRightClick.value())));
}

void MainWindow::onNavigationOpenContextMenu() { navigationMenu->exec(QCursor::pos()); }

void MainWindow::onNavigationCloseContextMenu() { rubberBandManager.hide(); }

void MainWindow::onNavigationTriggerContextMenuEntry()
{
  auto *action = qobject_cast<QAction *>(sender());
  if (!action || !action->property("id").isValid()) return;

  Dock *dock = action->property("id").value<Dock *>();
  assert(dock != nullptr);

  dock->raise();
  dock->show();
  dock->setFocus();

  // Forward the focus on the content of the tabmanager
  if (dock == editorDock) {
    tabManager->setFocus();
  }
}

void MainWindow::onNavigationHoveredContextMenuEntry()
{
  auto *action = qobject_cast<QAction *>(sender());
  if (!action || !action->property("id").isValid()) return;

  Dock *dock = action->property("id").value<Dock *>();
  assert(dock != nullptr);

  // Hover signal is emitted at each mouse move, to avoid excessive
  // load we only raise/emphasize if it is not yet done.
  if (rubberBandManager.isEmphasized(dock)) return;

  dock->raise();
  rubberBandManager.emphasize(dock);
}

void MainWindow::addExportActions(QToolBar *toolbar, QAction *action) const
{
  for (const std::string& identifier :
       {Settings::Settings::toolbarExport3D.value(), Settings::Settings::toolbarExport2D.value()}) {
    QAction *exportAction = formatIdentifierToAction(identifier);
    if (exportAction) {
      toolbar->insertAction(action, exportAction);
    }
  }
}

void MainWindow::updateExportActions()
{
  removeExportActions(editortoolbar, this->designAction3DPrint);
  addExportActions(editortoolbar, this->designAction3DPrint);

  // handle the hide/show of export action in view toolbar according to the visibility of editor dock
  removeExportActions(viewerToolBar, this->viewActionViewAll);
  if (!editorDock->isVisible()) {
    addExportActions(viewerToolBar, this->viewActionViewAll);
  }
}

void MainWindow::openFileFromPath(const QString& path, int line)
{
  if (editorDock->isVisible()) {
    activeEditor->setFocus();
    if (!path.isEmpty()) tabManager->open(path);
    activeEditor->setFocus();
    activeEditor->setCursorPosition(line, 0);
  }
}

void MainWindow::addKeyboardShortCut(const QList<QAction *>& actions)
{
  for (auto& action : actions) {
    // prevent adding shortcut twice if action is added to multiple toolbars
    if (action->toolTip().contains("&nbsp;")) {
      continue;
    }

    const QString shortCut(action->shortcut().toString(QKeySequence::NativeText));
    if (shortCut.isEmpty()) {
      continue;
    }

    const QString toolTip(
      "%1 &nbsp;<span style=\"color: gray; font-size: small; font-style: italic\">%2</span>");
    action->setToolTip(toolTip.arg(action->toolTip(), shortCut));
  }
}

/**
 * Update window settings that get overwritten by the restoreState()
 * Qt call. So the values are loaded before the call and restored here
 * regardless of the (potential outdated) serialized state.
 */
void MainWindow::updateWindowSettings(bool isEditorToolbarVisible, bool isViewToolbarVisible)
{
  viewActionHideEditorToolBar->setChecked(!isEditorToolbarVisible);
  hideEditorToolbar();
  viewActionHide3DViewToolBar->setChecked(!isViewToolbarVisible);
  hide3DViewToolbar();
}

void MainWindow::onAxisChanged(InputEventAxisChanged *) {}

void MainWindow::onButtonChanged(InputEventButtonChanged *) {}

void MainWindow::onTranslateEvent(InputEventTranslate *event)
{
  const double zoomFactor = 0.001 * qglview->cam.zoomValue();

  if (event->viewPortRelative) {
    qglview->translate(event->x, event->y, event->z, event->relative, true);
  } else {
    qglview->translate(zoomFactor * event->x, event->y, zoomFactor * event->z, event->relative, false);
  }
}

void MainWindow::onRotateEvent(InputEventRotate *event)
{
  qglview->rotate(event->x, event->y, event->z, event->relative);
}

void MainWindow::onRotate2Event(InputEventRotate2 *event)
{
  qglview->rotate2(event->x, event->y, event->z);
}

void MainWindow::onActionEvent(InputEventAction *event)
{
  const std::string actionName = event->action;
  if (actionName.find("::") == std::string::npos) {
    QAction *action = findAction(this->menuBar()->actions(), actionName);
    if (action) {
      action->trigger();
    } else if ("viewActionTogglePerspective" == actionName) {
      viewTogglePerspective();
    }
  } else {
    const std::string target = actionName.substr(0, actionName.find("::"));
    if (target == "animate") {
      this->animateWidget->onActionEvent(event);
    } else {
      std::cout << "unknown onActionEvent target: " << actionName << std::endl;
    }
  }
}

void MainWindow::onZoomEvent(InputEventZoom *event) { qglview->zoom(event->zoom, event->relative); }

void MainWindow::loadViewSettings()
{
  const QSettingsCached settings;

  if (settings.value("view/showEdges").toBool()) {
    viewActionShowEdges->setChecked(true);
    viewModeShowEdges();
  }
  if (settings.value("view/showAxes", true).toBool()) {
    viewActionShowAxes->setChecked(true);
    viewModeShowAxes();
  }
  if (settings.value("view/showCrosshairs").toBool()) {
    viewActionShowCrosshairs->setChecked(true);
    viewModeShowCrosshairs();
  }
  if (settings.value("view/showScaleProportional", true).toBool()) {
    viewActionShowScaleProportional->setChecked(true);
    viewModeShowScaleProportional();
  }
  if (settings.value("view/orthogonalProjection").toBool()) {
    viewOrthogonal();
  } else {
    viewPerspective();
  }

  updateUndockMode(GlobalPreferences::inst()->getValue("advanced/undockableWindows").toBool());
  updateReorderMode(GlobalPreferences::inst()->getValue("advanced/reorderWindows").toBool());
}

void MainWindow::loadDesignSettings()
{
  const QSettingsCached settings;
  if (settings.value("design/autoReload", false).toBool()) {
    designActionAutoReload->setChecked(true);
  }
  auto polySetCacheSizeMB = GlobalPreferences::inst()->getValue("advanced/polysetCacheSizeMB").toUInt();
  GeometryCache::instance()->setMaxSizeMB(polySetCacheSizeMB);
  auto cgalCacheSizeMB = GlobalPreferences::inst()->getValue("advanced/cgalCacheSizeMB").toUInt();
  CGALCache::instance()->setMaxSizeMB(cgalCacheSizeMB);
  auto backend3D =
    GlobalPreferences::inst()->getValue("advanced/renderBackend3D").toString().toStdString();
  RenderSettings::inst()->backend3D =
    renderBackend3DFromString(backend3D).value_or(DEFAULT_RENDERING_BACKEND_3D);
}

void MainWindow::updateUndockMode(bool undockMode)
{
  MainWindow::undockMode = undockMode;
  if (undockMode) {
    editorDock->setFeatures(editorDock->features() | QDockWidget::DockWidgetFloatable);
    consoleDock->setFeatures(consoleDock->features() | QDockWidget::DockWidgetFloatable);
    parameterDock->setFeatures(parameterDock->features() | QDockWidget::DockWidgetFloatable);
    errorLogDock->setFeatures(errorLogDock->features() | QDockWidget::DockWidgetFloatable);
    animateDock->setFeatures(animateDock->features() | QDockWidget::DockWidgetFloatable);
    fontListDock->setFeatures(fontListDock->features() | QDockWidget::DockWidgetFloatable);
    colorListDock->setFeatures(colorListDock->features() | QDockWidget::DockWidgetFloatable);
    viewportControlDock->setFeatures(viewportControlDock->features() | QDockWidget::DockWidgetFloatable);
  } else {
    if (editorDock->isFloating()) {
      editorDock->setFloating(false);
    }
    editorDock->setFeatures(editorDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (consoleDock->isFloating()) {
      consoleDock->setFloating(false);
    }
    consoleDock->setFeatures(consoleDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (parameterDock->isFloating()) {
      parameterDock->setFloating(false);
    }
    parameterDock->setFeatures(parameterDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (errorLogDock->isFloating()) {
      errorLogDock->setFloating(false);
    }
    errorLogDock->setFeatures(errorLogDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (animateDock->isFloating()) {
      animateDock->setFloating(false);
    }
    animateDock->setFeatures(animateDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (fontListDock->isFloating()) {
      fontListDock->setFloating(false);
    }
    fontListDock->setFeatures(fontListDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (colorListDock->isFloating()) {
      colorListDock->setFloating(false);
    }
    colorListDock->setFeatures(colorListDock->features() & ~QDockWidget::DockWidgetFloatable);

    if (viewportControlDock->isFloating()) {
      viewportControlDock->setFloating(false);
    }
    viewportControlDock->setFeatures(viewportControlDock->features() &
                                     ~QDockWidget::DockWidgetFloatable);
  }
}

void MainWindow::updateReorderMode(bool reorderMode)
{
  MainWindow::reorderMode = reorderMode;
  for (auto& [dock, name, configKey] : docks) {
    dock->setTitleBarVisibility(!reorderMode);
  }
}

MainWindow::~MainWindow()
{
  scadApp->windowManager.remove(this);
  if (scadApp->windowManager.getWindows().empty()) {
    // Quit application even in case some other windows like
    // Preferences are still open.
    scadApp->quit();
  }
}

void MainWindow::showProgress() { updateStatusBar(qobject_cast<ProgressWidget *>(sender())); }

void MainWindow::report_func(const std::shared_ptr<const AbstractNode>&, void *vp, int mark)
{
  // limit to progress bar update calls to 5 per second
  static const qint64 MIN_TIMEOUT = 200;
  if (progressThrottle->hasExpired(MIN_TIMEOUT)) {
    progressThrottle->start();

    auto thisp = static_cast<MainWindow *>(vp);
    auto v = static_cast<int>((mark * 1000.0) / progress_report_count);
    auto permille = v < 1000 ? v : 999;
    if (permille > thisp->progresswidget->value()) {
      QMetaObject::invokeMethod(thisp->progresswidget, "setValue", Qt::QueuedConnection,
                                Q_ARG(int, permille));
      QApplication::processEvents();
    }

    // FIXME: Check if cancel was requested by e.g. Application quit
    if (thisp->progresswidget->wasCanceled()) throw ProgressCancelException();
  }
}

bool MainWindow::network_progress_func(const double permille)
{
  QMetaObject::invokeMethod(this->progresswidget, "setValue", Qt::QueuedConnection,
                            Q_ARG(int, (int)permille));
  return (progresswidget && progresswidget->wasCanceled());
}

void MainWindow::updateRecentFiles(const QString& FileSavedOrOpened)
{
  // Check that the canonical file path exists - only update recent files
  // if it does. Should prevent empty list items on initial open etc.
  QSettingsCached settings;  // already set up properly via main.cpp
  auto files = settings.value("recentFileList").toStringList();
  files.removeAll(FileSavedOrOpened);
  files.prepend(FileSavedOrOpened);
  while (files.size() > UIUtils::maxRecentFiles) files.removeLast();
  settings.setValue("recentFileList", files);

  for (auto& widget : QApplication::topLevelWidgets()) {
    auto mainWin = qobject_cast<MainWindow *>(widget);
    if (mainWin) {
      mainWin->updateRecentFileActions();
    }
  }
}

/*!
   compiles the design. Calls compileDone() if anything was compiled
 */
void MainWindow::compile(bool reload, bool forcedone)
{
  OpenSCAD::hardwarnings = GlobalPreferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  OpenSCAD::traceDepth = GlobalPreferences::inst()->getValue("advanced/traceDepth").toUInt();
  OpenSCAD::traceUsermoduleParameters =
    GlobalPreferences::inst()->getValue("advanced/enableTraceUsermoduleParameters").toBool();
  OpenSCAD::parameterCheck =
    GlobalPreferences::inst()->getValue("advanced/enableParameterCheck").toBool();
  OpenSCAD::rangeCheck =
    GlobalPreferences::inst()->getValue("advanced/enableParameterRangeCheck").toBool();

  try {
    bool shouldcompiletoplevel = false;
    bool didcompile = false;

    compileErrors = 0;
    compileWarnings = 0;

    this->renderStatistic.start();

    // Reload checks the timestamp of the toplevel file and refreshes if necessary,
    if (reload) {
      // Refresh files if it has changed on disk
      if (fileChangedOnDisk() && checkEditorModified()) {
        shouldcompiletoplevel =
          tabManager->refreshDocument();  // don't compile if we couldn't open the file
        if (shouldcompiletoplevel &&
            GlobalPreferences::inst()->getValue("advanced/autoReloadRaise").toBool()) {
          // reloading the 'same' document brings the 'old' one to front.
          this->raise();
        }
      }
      // If the file has some content and there is no currently compiled content,
      // then we force the top level compilation.
      else {
        auto current_doc = activeEditor->toPlainText();
        if (current_doc.size() && lastCompiledDoc.size() == 0) {
          shouldcompiletoplevel = true;
        }
      }
    } else {
      shouldcompiletoplevel = true;
    }

    if (this->parsedFile) {
      auto mtime = this->parsedFile->includesChanged();
      if (mtime > this->includesMTime) {
        this->includesMTime = mtime;
        shouldcompiletoplevel = true;
      }
    }

    // Parsing and dependency handling must run to completion even with stop on errors to prevent auto
    // reload picking up where it left off, thwarting the stop, so we turn off exceptions in PRINT.
    no_exceptions_for_warnings();
    if (shouldcompiletoplevel) {
      initialize_rng();
      this->errorLogWidget->clearModel();
      if (GlobalPreferences::inst()->getValue("advanced/consoleAutoClear").toBool()) {
        this->console->actionClearConsole_triggered();
      }
      if (activeEditor->isContentModified()) saveBackup();
      parseTopLevelDocument();
      didcompile = true;
    }

    if (didcompile && parser_error_pos != lastParserErrorPos) {
      if (lastParserErrorPos >= 0) emit unhighlightLastError();
      if (parser_error_pos >= 0) emit highlightError(parser_error_pos);
      lastParserErrorPos = parser_error_pos;
    }

    if (this->rootFile) {
      auto mtime = this->rootFile->handleDependencies();
      if (mtime > this->depsMTime) {
        this->depsMTime = mtime;
        LOG("Used file cache size: %1$d files", SourceFileCache::instance()->size());
        didcompile = true;
      }
    }

    // Had any errors in the parse that would have caused exceptions via PRINT.
    if (would_have_thrown()) throw HardWarningException("");
    // If we're auto-reloading, listen for a cascade of changes by starting a timer
    // if something changed _and_ there are any external dependencies
    if (reload && didcompile && this->rootFile) {
      if (this->rootFile->hasIncludes() || this->rootFile->usesLibraries()) {
        this->waitAfterReloadTimer->start();
        this->procevents = false;
        return;
      }
    }

    compileDone(didcompile | forcedone);
  } catch (const HardWarningException&) {
    exceptionCleanup();
  } catch (const std::exception& ex) {
    UnknownExceptionCleanup(ex.what());
  } catch (...) {
    UnknownExceptionCleanup();
  }
}

void MainWindow::waitAfterReload()
{
  no_exceptions_for_warnings();
  auto mtime = this->rootFile->handleDependencies();
  auto stop = would_have_thrown();
  if (mtime > this->depsMTime) this->depsMTime = mtime;
  else if (!stop) {
    compile(true, true);  // In case file itself or top-level includes changed during dependency updates
    return;
  }
  this->waitAfterReloadTimer->start();
}

void MainWindow::on_toolButtonCompileResultClose_clicked() { frameCompileResult->hide(); }

void MainWindow::updateCompileResult()
{
  if ((compileErrors == 0) && (compileWarnings == 0)) {
    frameCompileResult->hide();
    return;
  }

  if (!Settings::Settings::showWarningsIn3dView.value()) {
    return;
  }

  QString msg;
  if (compileErrors > 0) {
    if (activeEditor->filepath.isEmpty()) {
      msg = QString(_("Compile error."));
    } else {
      const QFileInfo fileInfo(activeEditor->filepath);
      msg = QString(_("Error while compiling '%1'.")).arg(fileInfo.fileName());
    }
    toolButtonCompileResultIcon->setIcon(
      QIcon(QString::fromUtf8(":/icons/information-icons-error.png")));
  } else {
    const char *fmt = ngettext("Compilation generated %1 warning.", "Compilation generated %1 warnings.",
                               compileWarnings);
    msg = QString(fmt).arg(compileWarnings);
    toolButtonCompileResultIcon->setIcon(
      QIcon(QString::fromUtf8(":/icons/information-icons-warning.png")));
  }
  const QFontMetrics fm(labelCompileResultMessage->font());
  const int sizeIcon = std::max(12, std::min(32, fm.height()));
  const int sizeClose = std::max(10, std::min(32, fm.height()) - 4);
  toolButtonCompileResultIcon->setIconSize(QSize(sizeIcon, sizeIcon));
  toolButtonCompileResultClose->setIconSize(QSize(sizeClose, sizeClose));

  msg += _(
    R"( For details see the <a href="#errorlog">error log</a> and <a href="#console">console window</a>.)");
  labelCompileResultMessage->setText(msg);
  frameCompileResult->show();
}

void MainWindow::compileDone(bool didchange)
{
  OpenSCAD::hardwarnings = GlobalPreferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  try {
    const char *callslot;
    if (didchange) {
      instantiateRoot();
      updateCompileResult();
      callslot = afterCompileSlot;
    } else {
      callslot = "compileEnded";
    }

    this->procevents = false;
    QMetaObject::invokeMethod(this, callslot);
  } catch (const HardWarningException&) {
    exceptionCleanup();
  }
}

void MainWindow::compileEnded()
{
  clearCurrentOutput();
  GuiLocker::unlock();
  if (designActionAutoReload->isChecked()) autoReloadTimer->start();
#ifdef ENABLE_GUI_TESTS
  emit compilationDone(this->rootFile);
#endif
}

#ifdef ENABLE_GUI_TESTS
std::shared_ptr<AbstractNode> MainWindow::instantiateRootFromSource(SourceFile *file)
{
  EvaluationSession session{file->getFullpath()};
  ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
  setRenderVariables(builtin_context);

  std::shared_ptr<const FileContext> file_context;
  std::shared_ptr<AbstractNode> node = this->rootFile->instantiate(*builtin_context, &file_context);

  return node;
}
#endif  // ifdef ENABLE_GUI_TESTS

void MainWindow::instantiateRoot()
{
  // Go on and instantiate root_node, then call the continuation slot

  // Invalidate renderers before we kill the CSG tree
  this->qglview->setRenderer(nullptr);
#ifdef ENABLE_OPENCSG
  this->previewRenderer = nullptr;
#endif
  this->thrownTogetherRenderer = nullptr;

  // Remove previous CSG tree
  this->absoluteRootNode.reset();

  this->csgRoot.reset();
  this->normalizedRoot.reset();
  this->rootProduct.reset();

  this->rootNode.reset();
  this->tree.setRoot(nullptr);

  const std::filesystem::path doc(activeEditor->filepath.toStdString());
  this->tree.setDocumentPath(doc.parent_path().string());

  renderedEditor = activeEditor;

  if (this->rootFile) {
    // Evaluate CSG tree
    LOG("Compiling design (CSG Tree generation)...");
    this->processEvents();

    AbstractNode::resetIndexCounter();

    EvaluationSession session{doc.parent_path().string()};
    ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
    setRenderVariables(builtin_context);

    std::shared_ptr<const FileContext> file_context;
#ifdef ENABLE_PYTHON
    if (python_result_node != NULL && this->python_active) this->absoluteRootNode = python_result_node;
    else
#endif
      this->absoluteRootNode = this->rootFile->instantiate(*builtin_context, &file_context);
    if (file_context) {
      this->qglview->cam.updateView(file_context, false);
      viewportControlWidget->cameraChanged();
    }

    if (this->absoluteRootNode) {
      // Do we have an explicit root node (! modifier)?
      const Location *nextLocation = nullptr;
      if (!(this->rootNode = find_root_tag(this->absoluteRootNode, &nextLocation))) {
        this->rootNode = this->absoluteRootNode;
      }
      if (nextLocation) {
        LOG(message_group::NONE, *nextLocation, builtin_context->documentRoot(),
            "More than one Root Modifier (!)");
      }

      // FIXME: Consider giving away ownership of root_node to the Tree, or use reference counted
      // pointers
      this->tree.setRoot(this->rootNode);
    }
  }

  if (!this->rootNode) {
    if (parser_error_pos < 0) {
      LOG(message_group::Error, "Compilation failed! (no top level object found)");
    } else {
      LOG(message_group::Error, "Compilation failed!");
    }
    LOG(" ");
    this->processEvents();
  }
}

/*!
   Generates CSG tree for OpenCSG evaluation.
   Assumes that the design has been parsed and evaluated (this->root_node is set)
 */
void MainWindow::compileCSG()
{
  OpenSCAD::hardwarnings = GlobalPreferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  try {
    assert(this->rootNode);
    LOG("Compiling design (CSG Products generation)...");
    this->processEvents();

    // Main CSG evaluation
    this->progresswidget = new ProgressWidget(this);
    connect(this->progresswidget, &ProgressWidget::requestShow, this, &MainWindow::showProgress);

    GeometryEvaluator geomevaluator(this->tree);
#ifdef ENABLE_OPENCSG
    CSGTreeEvaluator csgrenderer(this->tree, &geomevaluator);
#endif

    if (!isClosing) progress_report_prep(this->rootNode, report_func, this);
    else return;
    try {
#ifdef ENABLE_OPENCSG
      this->processEvents();
      this->csgRoot = csgrenderer.buildCSGTree(*rootNode);
#endif
      renderStatistic.printCacheStatistic();
      this->processEvents();
    } catch (const ProgressCancelException&) {
      LOG("CSG generation cancelled.");
    } catch (const HardWarningException&) {
      LOG("CSG generation cancelled due to hardwarning being enabled.");
    }
    progress_report_fin();
    updateStatusBar(nullptr);

    LOG("Compiling design (CSG Products normalization)...");
    this->processEvents();

    const size_t normalizelimit =
      2ul * GlobalPreferences::inst()->getValue("advanced/openCSGLimit").toUInt();
    CSGTreeNormalizer normalizer(normalizelimit);

    if (this->csgRoot) {
      this->normalizedRoot = normalizer.normalize(this->csgRoot);
      if (this->normalizedRoot) {
        this->rootProduct = std::make_shared<CSGProducts>();
        this->rootProduct->import(this->normalizedRoot);
      } else {
        this->rootProduct.reset();
        LOG(message_group::Warning, "CSG normalization resulted in an empty tree");
        this->processEvents();
      }
    }

    const std::vector<std::shared_ptr<CSGNode>>& highlight_terms = csgrenderer.getHighlightNodes();
    if (highlight_terms.size() > 0) {
      LOG("Compiling highlights (%1$d CSG Trees)...", highlight_terms.size());
      this->processEvents();

      this->highlightsProducts = std::make_shared<CSGProducts>();
      for (const auto& highlight_term : highlight_terms) {
        auto nterm = normalizer.normalize(highlight_term);
        if (nterm) {
          this->highlightsProducts->import(nterm);
        }
      }
    } else {
      this->highlightsProducts.reset();
    }

    const auto& background_terms = csgrenderer.getBackgroundNodes();
    if (background_terms.size() > 0) {
      LOG("Compiling background (%1$d CSG Trees)...", background_terms.size());
      this->processEvents();

      this->backgroundProducts = std::make_shared<CSGProducts>();
      for (const auto& background_term : background_terms) {
        auto nterm = normalizer.normalize(background_term);
        if (nterm) {
          this->backgroundProducts->import(nterm);
        }
      }
    } else {
      this->backgroundProducts.reset();
    }

    if (this->rootProduct && (this->rootProduct->size() >
                              GlobalPreferences::inst()->getValue("advanced/openCSGLimit").toUInt())) {
      LOG(message_group::UI_Warning, "Normalized tree has %1$d elements!", this->rootProduct->size());
      LOG(message_group::UI_Warning, "OpenCSG rendering has been disabled.");
    }
#ifdef ENABLE_OPENCSG
    else {
      LOG("Normalized tree has %1$d elements!", (this->rootProduct ? this->rootProduct->size() : 0));
      this->previewRenderer = std::make_shared<OpenCSGRenderer>(
        this->rootProduct, this->highlightsProducts, this->backgroundProducts);
    }
#endif  // ifdef ENABLE_OPENCSG
    this->thrownTogetherRenderer = std::make_shared<ThrownTogetherRenderer>(
      this->rootProduct, this->highlightsProducts, this->backgroundProducts);
    LOG("Compile and preview finished.");
    renderStatistic.printRenderingTime();
    this->processEvents();
  } catch (const HardWarningException&) {
    exceptionCleanup();
  }
}

void MainWindow::actionOpen()
{
  auto fileInfoList = UIUtils::openFiles(this);
  for (auto& i : fileInfoList) {
    if (!i.exists()) {
      return;
    }
    tabManager->open(i.filePath());
  }
}

void MainWindow::actionNewWindow() { new MainWindow(QStringList()); }

void MainWindow::actionOpenWindow()
{
  auto fileInfoList = UIUtils::openFiles(this);
  for (auto& i : fileInfoList) {
    if (!i.exists()) {
      return;
    }
    new MainWindow(QStringList(i.filePath()));
  }
}

void MainWindow::actionOpenRecent()
{
  auto action = qobject_cast<QAction *>(sender());
  tabManager->open(action->data().toString());
}

void MainWindow::clearRecentFiles()
{
  QSettingsCached settings;  // already set up properly via main.cpp
  const QStringList files;
  settings.setValue("recentFileList", files);

  updateRecentFileActions();
}

// Updates the content of the recent files menu entries
// by iterating over the recently opened files.
void MainWindow::updateRecentFileActions()
{
  auto files = UIUtils::recentFiles();

  for (int i = 0; i < files.size(); ++i) {
    this->actionRecentFile[i]->setText(QFileInfo(files[i]).fileName().replace("&", "&&"));
    this->actionRecentFile[i]->setData(files[i]);
    this->actionRecentFile[i]->setVisible(true);
  }
  for (int i = files.size(); i < UIUtils::maxRecentFiles; ++i) {
    this->actionRecentFile[i]->setVisible(false);
  }
}

void MainWindow::show_examples()
{
  bool found_example = false;

  for (const auto& cat : UIUtils::exampleCategories()) {
    auto examples = UIUtils::exampleFiles(cat.name);
    auto menu = this->menuExamples->addMenu(gettext(cat.name.toStdString().c_str()));
    if (!cat.tooltip.trimmed().isEmpty()) {
      menu->setToolTip(gettext(cat.tooltip.toStdString().c_str()));
      menu->setToolTipsVisible(true);
    }

    for (const auto& ex : examples) {
      auto openAct = new QAction(ex.fileName().replace("&", "&&"), this);
      connect(openAct, &QAction::triggered, this, &MainWindow::actionOpenExample);
      menu->addAction(openAct);
      openAct->setData(ex.canonicalFilePath());
      found_example = true;
    }
  }

  if (!found_example) {
    delete this->menuExamples;
    this->menuExamples = nullptr;
  }
}

void MainWindow::actionOpenExample()
{
  const auto action = qobject_cast<QAction *>(sender());
  if (action) {
    const auto& path = action->data().toString();
    tabManager->open(path);
  }
}

void MainWindow::writeBackup(QFile *file)
{
  // see MainWindow::saveBackup()
  file->resize(0);
  QTextStream writer(file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  writer.setCodec("UTF-8");
#endif
  writer << activeEditor->toPlainText();
  this->activeEditor->parameterWidget->saveBackupFile(file->fileName());

  LOG("Saved backup file: %1$s", file->fileName().toUtf8().constData());
}

void MainWindow::saveBackup()
{
  auto path = PlatformUtils::backupPath();
  if ((!fs::exists(path)) && (!PlatformUtils::createBackupPath())) {
    LOG(message_group::UI_Warning, "Cannot create backup path: %1$s", path);
    return;
  }

  auto backupPath = QString::fromLocal8Bit(path.c_str());
  if (!backupPath.endsWith("/")) backupPath.append("/");

  QString basename = "unsaved";
  if (!activeEditor->filepath.isEmpty()) {
    auto fileInfo = QFileInfo(activeEditor->filepath);
    basename = fileInfo.baseName();
  }

  if (!this->tempFile) {
#ifdef ENABLE_PYTHON
    const QString suffix = this->python_active ? "py" : "scad";
#else
    const QString suffix = "scad";
#endif
    this->tempFile = new QTemporaryFile(backupPath.append(basename + "-backup-XXXXXXXX." + suffix));
  }

  if ((!this->tempFile->isOpen()) && (!this->tempFile->open())) {
    LOG(message_group::UI_Warning, "Failed to create backup file");
    return;
  }
  return writeBackup(this->tempFile);
}

void MainWindow::actionSave() { tabManager->save(activeEditor); }

void MainWindow::actionSaveAs() { tabManager->saveAs(activeEditor); }

void MainWindow::actionPythonRevokeTrustedFiles()
{
  QSettingsCached settings;
#ifdef ENABLE_PYTHON
  python_trusted = false;
  this->trusted_edit_document_name = "";
#endif
  settings.remove("python_hash");
  QMessageBox::information(this, _("Trusted Files"), "All trusted python files revoked",
                           QMessageBox::Ok);
}

void MainWindow::actionPythonCreateVenv()
{
#ifdef ENABLE_PYTHON
  const QString selectedDir = QFileDialog::getExistingDirectory(this, "Create Virtual Environment");
  if (selectedDir.isEmpty()) {
    return;
  }

  const QDir venvDir{selectedDir};
  if (!venvDir.exists()) {
    // Should not happen, but just in case double check...
    QMessageBox::critical(this, _("Create Virtual Environment"),
                          "Directory does not exist. Can't create virtual environment.",
                          QMessageBox::Ok);
    return;
  }

  if (!venvDir.isEmpty()) {
    QMessageBox::critical(this, _("Create Virtual Environment"),
                          "Directory is not empty. Can't create virtual environment.", QMessageBox::Ok);
    return;
  }

  const auto& path = venvDir.absolutePath().toStdString();
  LOG("Creating Python virtual environment in '%1$s'...", path);
  int result = pythonCreateVenv(path);

  if (result == 0) {
    Settings::SettingsPython::pythonVirtualEnv.setValue(path);
    Settings::Settings::visit(SettingsWriter());
    LOG("Python virtual environment creation successfull.");
    QMessageBox::information(this, _("Create Virtual Environment"),
                             "Virtual environment created, please restart OpenSCAD to activate.",
                             QMessageBox::Ok);
  } else {
    LOG("Python virtual environment creation failed.");
    QMessageBox::critical(this, _("Create Virtual Environment"), "Virtual environment creation failed.",
                          QMessageBox::Ok);
  }
#endif  // ifdef ENABLE_PYTHON
}

void MainWindow::actionPythonSelectVenv()
{
#ifdef ENABLE_PYTHON
  const QString venvDir = QFileDialog::getExistingDirectory(this, "Select Virtual Environment");
  if (venvDir.isEmpty()) {
    return;
  }
  const QFileInfo fileInfo{QDir{venvDir}, "pyvenv.cfg"};
  if (fileInfo.exists()) {
    Settings::SettingsPython::pythonVirtualEnv.setValue(venvDir.toStdString());
    Settings::Settings::visit(SettingsWriter());
    QMessageBox::information(this, _("Select Virtual Environment"),
                             "Virtual environment selected, please restart OpenSCAD to activate.",
                             QMessageBox::Ok);
  }
#endif  // ifdef ENABLE_PYTHON
}

void MainWindow::actionSaveACopy() { tabManager->saveACopy(activeEditor); }

void MainWindow::actionShowLibraryFolder()
{
  auto path = PlatformUtils::userLibraryPath();
  if (!fs::exists(path)) {
    LOG(message_group::UI_Warning, "Library path %1$s doesn't exist. Creating", path);
    if (!PlatformUtils::createUserLibraryPath()) {
      LOG(message_group::UI_Error, "Cannot create library path: %1$s", path);
    }
  }
  auto url = QString::fromStdString(path);
  LOG("Opening file browser for %1$s", url.toStdString());
  QDesktopServices::openUrl(QUrl::fromLocalFile(url));
}

void MainWindow::actionReload()
{
  if (checkEditorModified()) {
    fileChangedOnDisk();                  // force cached autoReloadId to update
    (void)tabManager->refreshDocument();  // ignore errors opening the file
  }
}

void MainWindow::copyViewportTranslation()
{
  const auto vpt = qglview->cam.getVpt();
  const QString txt =
    QString("[ %1, %2, %3 ]").arg(vpt.x(), 0, 'f', 2).arg(vpt.y(), 0, 'f', 2).arg(vpt.z(), 0, 'f', 2);
  QApplication::clipboard()->setText(txt);
}

void MainWindow::copyViewportRotation()
{
  const auto vpr = qglview->cam.getVpr();
  const QString txt =
    QString("[ %1, %2, %3 ]").arg(vpr.x(), 0, 'f', 2).arg(vpr.y(), 0, 'f', 2).arg(vpr.z(), 0, 'f', 2);
  QApplication::clipboard()->setText(txt);
}

void MainWindow::copyViewportDistance()
{
  const QString txt = QString::number(qglview->cam.zoomValue(), 'f', 2);
  QApplication::clipboard()->setText(txt);
}

void MainWindow::copyViewportFov()
{
  const QString txt = QString::number(qglview->cam.fovValue(), 'f', 2);
  QApplication::clipboard()->setText(txt);
}

QList<double> MainWindow::getTranslation() const
{
  QList<double> ret;
  ret.append(qglview->cam.object_trans.x());
  ret.append(qglview->cam.object_trans.y());
  ret.append(qglview->cam.object_trans.z());
  return ret;
}

QList<double> MainWindow::getRotation() const
{
  QList<double> ret;
  ret.append(qglview->cam.object_rot.x());
  ret.append(qglview->cam.object_rot.y());
  ret.append(qglview->cam.object_rot.z());
  return ret;
}

void MainWindow::hideFind()
{
  find_panel->hide();
  activeEditor->findState = TabManager::FIND_HIDDEN;
  editActionFindNext->setEnabled(false);
  editActionFindPrevious->setEnabled(false);
  this->findInputField->setFindCount(
    activeEditor->updateFindIndicators(this->findInputField->text(), false));
  this->processEvents();
}

// Prepare the UI for the find (and replace if requested)
// Among other thing it makes the text field and replacement field visible and well as it configures the
// activeEditor to appropriate search mode.
void MainWindow::showFind(bool doFindAndReplace)
{
  findInputField->setFindCount(activeEditor->updateFindIndicators(findInputField->text()));
  processEvents();

  if (doFindAndReplace) {
    findTypeComboBox->setCurrentIndex(1);
    replaceInputField->show();
    replaceButton->show();
    replaceAllButton->show();
    activeEditor->findState = TabManager::FIND_REPLACE_VISIBLE;
  } else {
    findTypeComboBox->setCurrentIndex(0);
    replaceInputField->hide();
    replaceButton->hide();
    replaceAllButton->hide();
    activeEditor->findState = TabManager::FIND_VISIBLE;
  }

  find_panel->show();
  editActionFindNext->setEnabled(true);
  editActionFindPrevious->setEnabled(true);
  if (!activeEditor->selectedText().isEmpty()) {
    findInputField->setText(activeEditor->selectedText());
  }
  findInputField->setFocus();
  findInputField->selectAll();
}

void MainWindow::actionShowFind() { showFind(false); }

void MainWindow::findString(const QString& textToFind)
{
  this->findInputField->setFindCount(activeEditor->updateFindIndicators(textToFind));
  this->processEvents();
  activeEditor->find(textToFind);
}

void MainWindow::actionShowFindAndReplace() { showFind(true); }

void MainWindow::actionSelectFind(int type)
{
  // If type is one, then we shows the find and replace UI component
  showFind(type == 1);
}

void MainWindow::replace()
{
  activeEditor->replaceSelectedText(this->replaceInputField->text());
  activeEditor->find(this->findInputField->text());
}

void MainWindow::replaceAll()
{
  activeEditor->replaceAll(this->findInputField->text(), this->replaceInputField->text());
}

void MainWindow::convertTabsToSpaces()
{
  const auto text = activeEditor->toPlainText();

  QString converted;

  int cnt = 4;
  for (auto c : text) {
    if (c == '\t') {
      for (; cnt > 0; cnt--) {
        converted.append(' ');
      }
    } else {
      converted.append(c);
    }
    if (cnt <= 0 || c == '\n') {
      cnt = 5;
    }
    cnt--;
  }
  activeEditor->setText(converted);
}

void MainWindow::findNext() { activeEditor->find(this->findInputField->text(), true); }

void MainWindow::findPrev() { activeEditor->find(this->findInputField->text(), true, true); }

void MainWindow::useSelectionForFind() { findInputField->setText(activeEditor->selectedText()); }

void MainWindow::updateFindBuffer(const QString& s)
{
  QApplication::clipboard()->setText(s, QClipboard::FindBuffer);
}

void MainWindow::findBufferChanged()
{
  auto t = QApplication::clipboard()->text(QClipboard::FindBuffer);
  // The convention seems to be to not update the search field if the findbuffer is empty
  if (!t.isEmpty()) {
    findInputField->setText(t);
  }
}

bool MainWindow::event(QEvent *event)
{
  if (event->type() == InputEvent::eventType) {
    auto *inputEvent = dynamic_cast<InputEvent *>(event);
    if (inputEvent) {
      inputEvent->deliver(this);
    }
    event->accept();
    return true;
  }
  return QMainWindow::event(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (rubberBandManager.isVisible()) {
    if (event->type() == QEvent::KeyRelease) {
      auto keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Control) {
        rubberBandManager.hide();
      }
    }
  }

  if (obj == find_panel) {
    if (event->type() == QEvent::KeyPress) {
      auto keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == Qt::Key_Escape) {
        this->hideFind();
        return true;
      }
    }
    return false;
  }

  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setRenderVariables(ContextHandle<BuiltinContext>& context)
{
  const RenderVariables r = {
    .preview = this->isPreview,
    .time = this->animateWidget->getAnimTval(),
    .camera = qglview->cam,
  };
  r.applyToContext(context);
}

/*!
   Returns true if the current document is a file on disk and that file has new content.
   Returns false if a file on disk has disappeared or if we haven't yet saved.
 */
bool MainWindow::fileChangedOnDisk()
{
  if (!activeEditor->filepath.isEmpty()) {
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    const bool valid = (stat(activeEditor->filepath.toLocal8Bit(), &st) == 0);
    // If file isn't there, just return and use current editor text
    if (!valid) return false;

    auto newid = str(boost::format("%x.%x") % st.st_mtime % st.st_size);
    if (newid != activeEditor->autoReloadId) {
      activeEditor->autoReloadId = newid;
      return true;
    }
  }
  return false;
}

/*!
   Returns true if anything was compiled.
 */

#ifdef ENABLE_PYTHON
bool MainWindow::trust_python_file(const std::string& file, const std::string& content)
{
  QSettingsCached settings;
  char setting_key[256];
  if (python_trusted) return true;

  std::string act_hash, ref_hash;
  snprintf(setting_key, sizeof(setting_key) - 1, "python_hash/%s", file.c_str());
  act_hash = SHA256HashString(content);

  if (file == this->untrusted_edit_document_name) return false;

  if (file == this->trusted_edit_document_name) {
    settings.setValue(setting_key, act_hash.c_str());
    return true;
  }

  if (content.size() <= 1) {  // 1st character already typed
    this->trusted_edit_document_name = file;
    return true;
  }
  if (content.rfind("from openscad import", 0) == 0) {  // 1st character already typed
    this->trusted_edit_document_name = file;
    return true;
  }

  if (settings.contains(setting_key)) {
    ref_hash = settings.value(setting_key).toString().toStdString();
  }

  if (act_hash == ref_hash) {
    this->trusted_edit_document_name = file;
    return true;
  }

  auto ret = QMessageBox::warning(this, "Application",
                                  _("Python files can potentially contain harmful stuff.\n"
                                    "Do you trust this file ?\n"),
                                  QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::No);
  if (ret == QMessageBox::YesAll) {
    python_trusted = true;
    return true;
  }
  if (ret == QMessageBox::Yes) {
    this->trusted_edit_document_name = file;
    settings.setValue(setting_key, act_hash.c_str());
    return true;
  }

  if (ret == QMessageBox::No) {
    this->untrusted_edit_document_name = file;
    return false;
  }
  return false;
}
#endif  // ifdef ENABLE_PYTHON

std::shared_ptr<SourceFile> MainWindow::parseDocument(EditorInterface *editor)
{
  resetSuppressedMessages();

  auto document = editor->toPlainText();
  auto fulltext = std::string(document.toUtf8().constData()) + "\n\x03\n" + commandline_commands;

  const std::string fname = editor->filepath.isEmpty() ? "" : editor->filepath.toStdString();
#ifdef ENABLE_PYTHON
  this->python_active = false;
  if (boost::algorithm::ends_with(fname, ".py")) {
    std::string content = std::string(this->lastCompiledDoc.toUtf8().constData());
    if (Feature::ExperimentalPythonEngine.is_enabled() && trust_python_file(fname, content))
      this->python_active = true;
    else LOG(message_group::Warning, Location::NONE, "", "Python is not enabled");
  }

  if (this->python_active) {
    auto fulltext_py = std::string(this->lastCompiledDoc.toUtf8().constData());

    const auto& venv = venvBinDirFromSettings();
    initPython(venv, this->animateWidget->getAnimTval());

    if (venv.empty()) {
      LOG("Running %1$s without venv.", python_version());
    } else {
      const auto& v = Settings::SettingsPython::pythonVirtualEnv.value();
      LOG("Running %1$s in venv '%2$s'.", python_version(), v);
    }
    auto error = evaluatePython(fulltext_py, false);
    if (error.size() > 0) LOG(message_group::Error, Location::NONE, "", error.c_str());
    fulltext = "\n";
  }
#endif  // ifdef ENABLE_PYTHON

  SourceFile *sourceFile;
  sourceFile = parse(sourceFile, fulltext, fname, fname, false) ? sourceFile : nullptr;

  editor->resetHighlighting();
  if (sourceFile) {
    // add parameters as annotation in AST
    CommentParser::collectParameters(fulltext, sourceFile);
    editor->parameterWidget->setParameters(sourceFile, fulltext);
    editor->parameterWidget->applyParameters(sourceFile);
    editor->parameterWidget->setEnabled(true);
    editor->setIndicator(sourceFile->indicatorData);
  } else {
    editor->parameterWidget->setEnabled(false);
  }

  return std::shared_ptr<SourceFile>(sourceFile);
}

void MainWindow::parseTopLevelDocument()
{
  resetSuppressedMessages();

  this->lastCompiledDoc = activeEditor->toPlainText();

  activeEditor->resetHighlighting();
  this->rootFile = parseDocument(activeEditor);
  this->parsedFile = this->rootFile;
}

void MainWindow::changeParameterWidget() { parameterDock->setVisible(true); }

void MainWindow::checkAutoReload()
{
  if (!activeEditor->filepath.isEmpty()) {
    actionReloadRenderPreview();
  }
}

void MainWindow::autoReloadSet(bool on)
{
  QSettingsCached settings;
  settings.setValue("design/autoReload", designActionAutoReload->isChecked());
  if (on) {
    autoReloadTimer->start(autoReloadPollingPeriodMS);
  } else {
    autoReloadTimer->stop();
  }
}

bool MainWindow::checkEditorModified()
{
  if (activeEditor->isContentModified()) {
    auto ret = QMessageBox::warning(this, _("Application"),
                                    _("The document has been modified.\n"
                                      "Do you really want to reload the file?"),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
      return false;
    }
  }
  return true;
}

void MainWindow::actionReloadRenderPreview()
{
  if (GuiLocker::isLocked()) return;
  GuiLocker::lock();
  autoReloadTimer->stop();
  setCurrentOutput();

  this->afterCompileSlot = "csgReloadRender";
  this->procevents = true;
  this->isPreview = true;
  compile(true);
}

void MainWindow::csgReloadRender()
{
  if (this->rootNode) compileCSG();

  // Go to non-CGAL view mode
  if (viewActionThrownTogether->isChecked()) {
    viewModeThrownTogether();
  } else {
#ifdef ENABLE_OPENCSG
    viewModePreview();
#else
    viewModeThrownTogether();
#endif
  }
  compileEnded();
}

void MainWindow::prepareCompile(const char *afterCompileSlot, bool procevents, bool preview)
{
  autoReloadTimer->stop();
  setCurrentOutput();
  LOG(" ");
  LOG("Parsing design (AST generation)...");
  this->processEvents();
  this->afterCompileSlot = afterCompileSlot;
  this->procevents = procevents;
  this->isPreview = preview;
}

void MainWindow::actionRenderPreview()
{
  static bool preview_requested;
  preview_requested = true;

  if (GuiLocker::isLocked()) return;

  GuiLocker::lock();
  preview_requested = false;

  resetMeasurementsState(false, "Render (not preview) to enable measurements");

  prepareCompile("csgRender", !animateDock->isVisible(), true);
  compile(false, false);

  if (preview_requested) {
    // if the action was called when the gui was locked, we must request it one more time
    // however, it's not possible to call it directly NOR make the loop
    // it must be called from the mainloop
    QTimer::singleShot(0, this, &MainWindow::actionRenderPreview);
    return;
  }
}

void MainWindow::csgRender()
{
  if (this->rootNode) compileCSG();

  // Go to non-CGAL view mode
  if (viewActionThrownTogether->isChecked()) {
    viewModeThrownTogether();
  } else {
#ifdef ENABLE_OPENCSG
    viewModePreview();
#else
    viewModeThrownTogether();
#endif
  }

  if (animateWidget->dumpPictures()) {
    const int steps = animateWidget->nextFrame();
    const QImage img = this->qglview->grabFrame();
    const QString filename = QString("frame%1.png").arg(steps, 5, 10, QChar('0'));
    img.save(filename, "PNG");
  }

  compileEnded();
}

void MainWindow::sendToExternalTool(ExternalToolInterface& externalToolService)
{
  const QFileInfo activeFile(activeEditor->filepath);
  QString activeFileName = activeFile.fileName();
  if (activeFileName.isEmpty()) activeFileName = "Untitled.scad";
  // TODO: Replace suffix to match exported file format?

  activeFileName = activeFileName +
                   QString::fromStdString("." + fileformat::toSuffix(externalToolService.fileFormat()));

  const bool export_status =
    externalToolService.exportTemporaryFile(rootGeom, activeFileName, &qglview->cam);
  if (!export_status) {
    return;
  }

  this->progresswidget = new ProgressWidget(this);
  connect(this->progresswidget, &ProgressWidget::requestShow, this, &MainWindow::showProgress);

  const bool process_status = externalToolService.process(
    activeFileName.toStdString(), [this](double permille) { return network_progress_func(permille); });
  updateStatusBar(nullptr);
  if (!process_status) {
    return;
  }

  const auto url = externalToolService.getURL();
  if (!url.empty()) {
    QDesktopServices::openUrl(QUrl{QString::fromStdString(url)});
  }
}

void MainWindow::action3DPrint()
{
  if (GuiLocker::isLocked()) return;
  const GuiLocker lock;

  setCurrentOutput();

  // Make sure we can export:
  const unsigned int dim = 3;
  if (!canExport(dim)) return;

  PrintInitDialog printInitDialog;
  const auto status = printInitDialog.exec();

  if (status == QDialog::Accepted) {
    const print_service_t serviceType = printInitDialog.getServiceType();
    const QString serviceName = printInitDialog.getServiceName();
    const FileFormat fileFormat = printInitDialog.getFileFormat();

    LOG("Selected File format: %1$s", fileformat::info(fileFormat).description);

    GlobalPreferences::inst()->updateGUI();
    const auto externalToolService = createExternalToolService(serviceType, serviceName, fileFormat);
    if (!externalToolService) {
      LOG("Error: Unable to create service: %1$d %2$s %3$d", static_cast<int>(serviceType),
          serviceName.toStdString(), static_cast<int>(fileFormat));
      return;
    }
    sendToExternalTool(*externalToolService);
  }
}

void MainWindow::actionRender()
{
  if (GuiLocker::isLocked()) return;
  GuiLocker::lock();

  prepareCompile("cgalRender", true, false);
  compile(false);
}

void MainWindow::cgalRender()
{
  if (!this->rootFile || !this->rootNode) {
    compileEnded();
    return;
  }

  this->qglview->setRenderer(nullptr);
  this->geomRenderer = nullptr;
  rootGeom.reset();

  LOG("Rendering Polygon Mesh using %1$s...",
      renderBackend3DToString(RenderSettings::inst()->backend3D).c_str());

  this->progresswidget = new ProgressWidget(this);
  connect(this->progresswidget, &ProgressWidget::requestShow, this, &MainWindow::showProgress);

  if (!isClosing) progress_report_prep(this->rootNode, report_func, this);
  else return;

  this->cgalworker->start(this->tree);
}

void MainWindow::actionRenderDone(const std::shared_ptr<const Geometry>& root_geom)
{
#ifdef ENABLE_PYTHON
  python_lock();
#endif
  progress_report_fin();
  if (root_geom) {
    std::vector<std::string> options;
    if (Settings::Settings::summaryCamera.value()) {
      options.emplace_back(RenderStatistic::CAMERA);
    }
    if (Settings::Settings::summaryArea.value()) {
      options.emplace_back(RenderStatistic::AREA);
    }
    if (Settings::Settings::summaryBoundingBox.value()) {
      options.emplace_back(RenderStatistic::BOUNDING_BOX);
    }
    renderStatistic.printAll(root_geom, qglview->cam, options);
    LOG("Rendering finished.");

    this->rootGeom = root_geom;
    // Choose PolySetRenderer for PolySet and Polygon2d, and for Manifold since we
    // know that all geometries are convertible to PolySet.
    if (RenderSettings::inst()->backend3D == RenderBackend3D::ManifoldBackend ||
        std::dynamic_pointer_cast<const PolySet>(this->rootGeom) ||
        std::dynamic_pointer_cast<const Polygon2d>(this->rootGeom)) {
      this->geomRenderer = std::make_shared<PolySetRenderer>(this->rootGeom);
    } else {
      this->geomRenderer = std::make_shared<CGALRenderer>(this->rootGeom);
    }

    // Go to CGAL view mode
    viewModeRender();
    resetMeasurementsState(true, "Click to start measuring");
  } else {
    resetMeasurementsState(false, "No top level geometry; render something to enable measurements");
    LOG(message_group::UI_Warning, "No top level geometry to render");
  }

  updateStatusBar(nullptr);

  const bool renderSoundEnabled =
    GlobalPreferences::inst()->getValue("advanced/enableSoundNotification").toBool();
  const uint soundThreshold =
    GlobalPreferences::inst()->getValue("advanced/timeThresholdOnRenderCompleteSound").toUInt();
  if (renderSoundEnabled && soundThreshold <= renderStatistic.ms().count() / 1000) {
    renderCompleteSoundEffect->play();
  }

  renderedEditor = activeEditor;
  activeEditor->contentsRendered = true;
  compileEnded();
}

void MainWindow::handleMeasurementClicked(QAction *clickedAction)
{
  // If we're unchecking, just stop.
  if (activeMeasurement == clickedAction) {
    resetMeasurementsState(true, "Click to start measuring");
    return;
  }

  resetMeasurementsState(true, "Click to start measuring");
  clickedAction->setToolTip("Click to cancel measurement");
  clickedAction->setChecked(true);
  activeMeasurement = clickedAction;

  if (clickedAction == designActionMeasureDist) {
    meas.startMeasureDist();
  } else if (clickedAction == designActionMeasureAngle) {
    meas.startMeasureAngle();
  }
}

void MainWindow::leftClick(QPoint mouse)
{
  std::vector<QString> strs = meas.statemachine(mouse);
  if (strs.size() > 0) {
    this->qglview->measure_state = MEASURE_DIRTY;
    QMenu resultmenu(this);
    // Ensures we clean the display regardless of how menu gets closed.
    connect(&resultmenu, &QMenu::aboutToHide, this, &MainWindow::measureFinished);

    // Can eventually be replaced with C++20 std::views::reverse
    for (const auto& str : boost::adaptors::reverse(strs)) {
      auto action = resultmenu.addAction(str);
      connect(action, &QAction::triggered, this, [str]() { QApplication::clipboard()->setText(str); });
    }
    resultmenu.addAction("Click any above to copy its text to the clipboard");
    resultmenu.exec(qglview->mapToGlobal(mouse));
    resetMeasurementsState(true, "Click to start measuring");
  }
}

/**
 * Call the mouseselection to determine the id of the clicked-on object.
 * Use the generated ID and try to find it within the list of products
 * And finally move the cursor to the beginning of the selected object in the editor
 */
void MainWindow::rightClick(QPoint position)
{
  // selecting without a renderer?!
  if (!this->qglview->renderer) {
    return;
  }
  // Nothing to select
  if (!this->rootProduct) {
    return;
  }

  // Select the object at mouse coordinates
  const int index = this->qglview->pickObject(position);
  std::deque<std::shared_ptr<const AbstractNode>> path;
  const std::shared_ptr<const AbstractNode> result = this->rootNode->getNodeByID(index, path);

  if (result) {
    // Create context menu with the backtrace
    QMenu tracemenu(this);
    std::stringstream ss;
    for (auto& step : path) {
      // Skip certain node types
      if (step->name() == "root") {
        continue;
      }
      auto location = step->modinst->location();
      ss.str("");

      // Remove the "module" prefix if any as it induce confusion between the module declaration and
      // instanciation
      const int first_position = (step->verbose_name().find("module") == std::string::npos) ? 0 : 7;
      std::string name = step->verbose_name().substr(first_position);

      // It happens that the verbose_name is empty (eg: in for loops), when this happens instead of
      // letting empty entry in the menu we prefer using the name in the modinstanciation.
      if (step->verbose_name().empty()) name = step->modinst->name();

      // Check if the path is contained in a library (using parsersettings.h)
      const fs::path libpath = get_library_for_path(location.filePath());
      if (!libpath.empty()) {
        // Display the library (without making the window too wide!)
        ss << name << " (library " << location.fileName().substr(libpath.string().length() + 1) << ":"
           << location.firstLine() << ")";
      } else if (renderedEditor->filepath.toStdString() == location.fileName()) {
        // removes the "module" prefix if any as it makes it not clear if it is module declaration or
        // call.
        ss << name << " (" << location.filePath().filename().string() << ":" << location.firstLine()
           << ")";
      } else {
        auto relative_filename =
          fs_uncomplete(location.filePath(),
                        fs::path(renderedEditor->filepath.toStdString()).parent_path())
            .generic_string();

        // Set the displayed name relative to the active editor window
        ss << name << " (" << relative_filename << ":" << location.firstLine() << ")";
      }
      // Prepare the action to be sent
      auto action = tracemenu.addAction(QString::fromStdString(ss.str()));
      if (editorDock->isVisible()) {
        action->setProperty("id", step->idx);
        connect(action, &QAction::hovered, this, &MainWindow::onHoveredObjectInSelectionMenu);
      }
    }

    // Before starting we need to lock the GUI to avoid interferance with reload/update
    // triggered by other part of the application (eg: changing the renderedEditor)
    GuiLocker::lock();

    // Execute this lambda function when the selection menu is closing.
    connect(&tracemenu, &QMenu::aboutToHide, [this]() {
      // remove the visual hints in the editor
      renderedEditor->clearAllSelectionIndicators();
      // unlock the GUI so the other part of the interface can now be updated.
      // (eg: changing the renderedEditor)
      GuiLocker::unlock();
    });
    tracemenu.exec(this->qglview->mapToGlobal(position));
  } else {
    clearAllSelectionIndicators();
  }
}

void MainWindow::measureFinished()
{
  auto didSomething = meas.stopMeasure();
  if (didSomething) resetMeasurementsState(true, "Click to start measuring");
}

void MainWindow::clearAllSelectionIndicators() { this->activeEditor->clearAllSelectionIndicators(); }

void MainWindow::setSelectionIndicatorStatus(EditorInterface *editor, int nodeIndex,
                                             EditorSelectionIndicatorStatus status)
{
  std::deque<std::shared_ptr<const AbstractNode>> stack;
  this->rootNode->getNodeByID(nodeIndex, stack);

  int level = 1;

  // first we flags all the nodes in the stack of the provided index
  // ends at size - 1 because we are not doing anything for the root node.
  // starts at 1 because we will process this one after later
  for (size_t i = 1; i < stack.size() - 1; i++) {
    const auto& node = stack[i];

    auto& location = node->modinst->location();
    if (location.filePath().compare(editor->filepath.toStdString()) != 0) {
      level++;
      continue;
    }

    if (node->verbose_name().rfind("module", 0) == 0 || node->modinst->name() == "children") {
      editor->setSelectionIndicatorStatus(status, level, location.firstLine() - 1,
                                          location.firstColumn() - 1, location.lastLine() - 1,
                                          location.lastColumn() - 1);
      level++;
    }
  }

  auto& node = stack[0];
  auto location = node->modinst->location();
  auto line = location.firstLine();
  auto column = location.firstColumn();
  auto lastLine = location.lastLine();
  auto lastColumn = location.lastColumn();

  // Update the location returned by location to cover the whole section.
  node->getCodeLocation(0, 0, &line, &column, &lastLine, &lastColumn, 0);

  editor->setSelectionIndicatorStatus(status, 0, line - 1, column - 1, lastLine - 1, lastColumn - 1);
}

void MainWindow::setSelection(int index)
{
  assert(renderedEditor != nullptr);
  if (currentlySelectedObject == index) return;

  std::deque<std::shared_ptr<const AbstractNode>> path;
  const std::shared_ptr<const AbstractNode> selected_node = rootNode->getNodeByID(index, path);

  if (!selected_node) return;

  currentlySelectedObject = index;

  auto location = selected_node->modinst->location();
  auto file = location.fileName();
  auto line = location.firstLine();
  auto column = location.firstColumn();

  // Unsaved files do have the pwd as current path, therefore we will not open a new
  // tab on click
  if (!fs::is_directory(fs::path(file))) {
    tabManager->open(QString::fromStdString(file));
  }

  // removes all previsly configure selection indicators.
  renderedEditor->clearAllSelectionIndicators();
  renderedEditor->show();

  std::vector<std::shared_ptr<const AbstractNode>> nodesSameModule{};
  rootNode->findNodesWithSameMod(selected_node, nodesSameModule);

  // highlight in the text editor all the text fragment of the hierarchy of object with same mode.
  for (const auto& element : nodesSameModule) {
    if (element->index() != currentlySelectedObject) {
      setSelectionIndicatorStatus(renderedEditor, element->index(),
                                  EditorSelectionIndicatorStatus::IMPACTED);
    }
  }

  // highlight in the text editor only the fragment correponding to the selected stack.
  // this step must be done after all the impacted element have been marked.
  setSelectionIndicatorStatus(renderedEditor, currentlySelectedObject,
                              EditorSelectionIndicatorStatus::SELECTED);

  renderedEditor->setCursorPosition(line - 1, column - 1);
}

/**
 * Expects the sender to have properties "id" defined
 */
void MainWindow::onHoveredObjectInSelectionMenu()
{
  assert(renderedEditor != nullptr);
  auto *action = qobject_cast<QAction *>(sender());
  if (!action || !action->property("id").isValid()) {
    return;
  }

  setSelection(action->property("id").toInt());
}

void MainWindow::setLastFocus(QWidget *widget) { this->lastFocus = widget; }

/**
 * Switch version label and progress widget. When switching to the progress
 * widget, the new instance is passed by the caller.
 * In case of resetting back to the version label, nullptr will be passed and
 * multiple calls can happen. So this method must guard against adding the
 * version label multiple times.
 *
 * @param progressWidget a pointer to the progress widget to show or nullptr in
 * case the display should switch back to the version label.
 */
void MainWindow::updateStatusBar(ProgressWidget *progressWidget)
{
  auto sb = this->statusBar();
  if (progressWidget == nullptr) {
    if (this->progresswidget != nullptr) {
      sb->removeWidget(this->progresswidget);
      delete this->progresswidget;
      this->progresswidget = nullptr;
    }
    if (versionLabel == nullptr) {
      versionLabel =
        new QLabel("OpenSCAD " + QString::fromStdString(std::string(openscad_displayversionnumber)));
      sb->addPermanentWidget(this->versionLabel);
    }
  } else {
    if (this->versionLabel != nullptr) {
      sb->removeWidget(this->versionLabel);
      delete this->versionLabel;
      this->versionLabel = nullptr;
    }
    sb->addPermanentWidget(progressWidget);
  }
}

void MainWindow::exceptionCleanup()
{
  LOG("Execution aborted");
  LOG(" ");
  GuiLocker::unlock();
  if (designActionAutoReload->isChecked()) autoReloadTimer->start();
}

void MainWindow::UnknownExceptionCleanup(std::string msg)
{
  setCurrentOutput();  // we need to show this error
  if (msg.size() == 0) {
    LOG(message_group::Error, "Compilation aborted by unknown exception");
  } else {
    LOG(message_group::Error, "Compilation aborted by exception: %1$s", msg);
  }
  LOG(" ");
  GuiLocker::unlock();
  if (designActionAutoReload->isChecked()) autoReloadTimer->start();
}

void MainWindow::showTextInWindow(const QString& type, const QString& content)
{
  auto e = new QTextEdit(this);
  e->setAttribute(Qt::WA_DeleteOnClose);
  e->setWindowFlags(Qt::Window);
  e->setTabStopDistance(tabStopWidth);
  e->setWindowTitle(type + " Dump");
  if (content.isEmpty()) e->setPlainText("No " + type + " to dump. Please try compiling first...");
  else e->setPlainText(content);

  e->setReadOnly(true);
  e->resize(600, 400);
  e->show();
}

void MainWindow::actionDisplayAST()
{
  setCurrentOutput();
  QString text = (rootFile) ? QString::fromStdString(rootFile->dump("")) : "";
  showTextInWindow("AST", text);
  clearCurrentOutput();
}

void MainWindow::actionDisplayCSGTree()
{
  setCurrentOutput();
  QString text = (rootNode) ? QString::fromStdString(tree.getString(*rootNode, "  ")) : "";
  showTextInWindow("CSG", text);
  clearCurrentOutput();
}

void MainWindow::actionDisplayCSGProducts()
{
  setCurrentOutput();
  // a small lambda to avoid code duplication
  auto constexpr dump = [](auto node) { return QString::fromStdString(node ? node->dump() : "N/A"); };
  auto text =
    QString(
      "\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering "
      "chain:\n%3\n\n\nHighlights CSG rendering chain:\n%4\n\n\nBackground CSG rendering chain:\n%5\n")
      .arg(dump(csgRoot), dump(normalizedRoot), dump(rootProduct), dump(highlightsProducts),
           dump(backgroundProducts));
  showTextInWindow("CSG Products Dump", text);
  clearCurrentOutput();
}

void MainWindow::actionCheckValidity()
{
  if (GuiLocker::isLocked()) return;
  const GuiLocker lock;
  setCurrentOutput();

  if (!rootGeom) {
    LOG("Nothing to validate! Try building first (press F6).");
    clearCurrentOutput();
    return;
  }

  if (rootGeom->getDimension() != 3) {
    LOG("Current top level object is not a 3D object.");
    clearCurrentOutput();
    return;
  }

  bool valid = true;
#ifdef ENABLE_CGAL
  if (auto N = std::dynamic_pointer_cast<const CGALNefGeometry>(rootGeom)) {
    valid = N->p3 ? const_cast<CGAL_Nef_polyhedron3&>(*N->p3).is_valid() : false;
  } else
#endif
#ifdef ENABLE_MANIFOLD
    if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(rootGeom)) {
    valid = mani->isValid();
  }
#endif
  LOG("Valid:      %1$6s", (valid ? "yes" : "no"));
  clearCurrentOutput();
}

// Returns if we can export (true) or not(false) (bool)
// Separated into it's own function for re-use.
bool MainWindow::canExport(unsigned int dim)
{
  if (!rootGeom) {
    LOG(message_group::Error, "Nothing to export! Try rendering first (press F6)");
    clearCurrentOutput();
    return false;
  }

  // editor has changed since last render
  if (!activeEditor->contentsRendered) {
    auto ret = QMessageBox::warning(this, "Application",
                                    "The current tab has been modified since its last render (F6).\n"
                                    "Do you really want to export the previous content?",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
      return false;
    }
  }

  // other tab contents most recently rendered
  if (renderedEditor != activeEditor) {
    auto ret = QMessageBox::warning(this, "Application",
                                    "The rendered data is of different tab.\n"
                                    "Do you really want to export the another tab's content?",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
      return false;
    }
  }

  if (rootGeom->getDimension() != dim) {
    LOG(message_group::UI_Error, "Current top level object is not a %1$dD object.", dim);
    clearCurrentOutput();
    return false;
  }

  if (rootGeom->isEmpty()) {
    LOG(message_group::UI_Error, "Current top level object is empty.");
    clearCurrentOutput();
    return false;
  }

#ifdef ENABLE_CGAL
  auto N = dynamic_cast<const CGALNefGeometry *>(rootGeom.get());
  if (N && !N->p3->is_simple()) {
    LOG(message_group::UI_Warning,
        "Object may not be a valid 2-manifold and may need repair! See "
        "https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
  }
#endif
#ifdef ENABLE_MANIFOLD
  auto manifold = dynamic_cast<const ManifoldGeometry *>(rootGeom.get());
  if (manifold && !manifold->isValid()) {
    LOG(message_group::UI_Warning,
        "Object may not be a valid manifold and may need repair! "
        "Error message: %1$s. See "
        "https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export",
        ManifoldUtils::statusToString(manifold->getManifold().Status()));
  }
#endif

  return true;
}

void MainWindow::actionExport(unsigned int dim, ExportInfo& exportInfo)
{
  const auto type_name = QString::fromStdString(exportInfo.info.description);
  const auto suffix = QString::fromStdString(exportInfo.info.suffix);

  // Setting filename skips the file selection dialog and uses the path provided instead.
  if (GuiLocker::isLocked()) return;
  const GuiLocker lock;

  setCurrentOutput();

  // Return if something is wrong and we can't export.
  if (!canExport(dim)) return;

  auto title = QString(_("Export %1 File")).arg(type_name);
  auto filter = QString(_("%1 Files (*%2)")).arg(type_name, suffix);
  auto exportFilename = QFileDialog::getSaveFileName(this, title, exportPath(suffix), filter);
  if (exportFilename.isEmpty()) {
    clearCurrentOutput();
    return;
  }
  this->exportPaths[suffix] = exportFilename;

  const bool exportResult = exportFileByName(rootGeom, exportFilename.toStdString(), exportInfo);

  if (exportResult) fileExportedMessage(type_name, exportFilename);
  clearCurrentOutput();
}

void MainWindow::actionExportFileFormat(int fmt)
{
  const auto format = static_cast<FileFormat>(fmt);
  const FileFormatInfo& info = fileformat::info(format);

  ExportInfo exportInfo =
    createExportInfo(format, info, activeEditor->filepath.toStdString(), &qglview->cam, {});

  switch (format) {
  case FileFormat::PDF: {
    ExportPdfDialog exportPdfDialog;
    if (exportPdfDialog.exec() == QDialog::Rejected) {
      return;
    }

    exportInfo.optionsPdf = exportPdfDialog.getOptions();
    actionExport(2, exportInfo);
  } break;
  case FileFormat::_3MF: {
    Export3mfDialog export3mfDialog;
    if (export3mfDialog.exec() == QDialog::Rejected) {
      return;
    }

    exportInfo.options3mf = export3mfDialog.getOptions();
    actionExport(3, exportInfo);
  } break;
  case FileFormat::CSG: {
    setCurrentOutput();

    if (!this->rootNode) {
      LOG(message_group::Error, "Nothing to export. Please try compiling first.");
      clearCurrentOutput();
      return;
    }
    const QString suffix = "csg";
    auto csg_filename = QFileDialog::getSaveFileName(this, _("Export CSG File"), exportPath(suffix),
                                                     _("CSG Files (*.csg)"));

    if (csg_filename.isEmpty()) {
      clearCurrentOutput();
      return;
    }

    std::ofstream fstream(std::filesystem::u8path(csg_filename.toStdString()));
    if (!fstream.is_open()) {
      LOG("Can't open file \"%1$s\" for export", csg_filename.toStdString());
    } else {
      fstream << this->tree.getString(*this->rootNode, "\t") << "\n";
      fstream.close();
      fileExportedMessage("CSG", csg_filename);
      this->exportPaths[suffix] = csg_filename;
    }

    clearCurrentOutput();
  } break;
  case FileFormat::PNG: {
    // Grab first to make sure dialog box isn't part of the grabbed image
    qglview->grabFrame();
    const QString suffix = "png";
    auto img_filename =
      QFileDialog::getSaveFileName(this, _("Export Image"), exportPath(suffix), _("PNG Files (*.png)"));
    if (!img_filename.isEmpty()) {
      const bool saveResult = qglview->save(img_filename.toStdString().c_str());
      if (saveResult) {
        this->exportPaths[suffix] = img_filename;
        setCurrentOutput();
        fileExportedMessage("PNG", img_filename);
        clearCurrentOutput();
      } else {
        LOG("Can't open file \"%1$s\" for export image", img_filename.toStdString());
      }
    }
  } break;
  case FileFormat::SVG: {
    ExportSvgDialog exportSvgDialog;
    if (exportSvgDialog.exec() == QDialog::Rejected) {
      return;
    }
    exportInfo.optionsSvg = std::make_shared<ExportSvgOptions>(exportSvgDialog.getOptions());
    actionExport(2, exportInfo);
  } break;
  default: actionExport(fileformat::is3D(format) ? 3 : fileformat::is2D(format) ? 2 : 0, exportInfo);
  }
}

void MainWindow::copyText()
{
  auto *c = dynamic_cast<Console *>(lastFocus);
  if (c) {
    c->copy();
  } else {
    tabManager->copy();
  }
}

void MainWindow::actionCopyViewport()
{
  const auto& image = qglview->grabFrame();
  auto clipboard = QApplication::clipboard();
  clipboard->setImage(image);
}

void MainWindow::actionFlushCaches()
{
  GeometryCache::instance()->clear();
  CGALCache::instance()->clear();
  dxf_dim_cache.clear();
  dxf_cross_cache.clear();
  SourceFileCache::instance()->clear();

  setCurrentOutput();
  LOG("Caches Flushed");
}

void MainWindow::viewModeActionsUncheck()
{
  viewActionPreview->setChecked(false);
  viewActionThrownTogether->setChecked(false);
}

#ifdef ENABLE_OPENCSG

void MainWindow::viewModeRender()
{
  viewActionThrownTogether->setEnabled(false);
  viewActionPreview->setEnabled(false);
  this->qglview->setRenderer(this->geomRenderer);
  this->qglview->updateColorScheme();
  this->qglview->update();
}

/*!
   Go to the OpenCSG view mode.
   Falls back to thrown together mode if OpenCSG is not available
 */
void MainWindow::viewModePreview()
{
  viewActionThrownTogether->setEnabled(true);
  viewActionPreview->setEnabled(this->qglview->hasOpenCSGSupport());
  if (this->qglview->hasOpenCSGSupport()) {
    viewActionPreview->setChecked(true);
    viewActionThrownTogether->setChecked(false);
    this->qglview->setRenderer(this->previewRenderer ? this->previewRenderer
                                                     : this->thrownTogetherRenderer);
    this->qglview->updateColorScheme();
    this->qglview->update();
  } else {
    viewModeThrownTogether();
  }
}

#endif /* ENABLE_OPENCSG */

void MainWindow::viewModeThrownTogether()
{
  viewActionThrownTogether->setEnabled(true);
  viewActionPreview->setEnabled(this->qglview->hasOpenCSGSupport());
  viewActionThrownTogether->setChecked(true);
  viewActionPreview->setChecked(false);
  this->qglview->setRenderer(this->thrownTogetherRenderer);
  this->qglview->updateColorScheme();
  this->qglview->update();
}

void MainWindow::viewModeShowEdges()
{
  QSettingsCached settings;
  settings.setValue("view/showEdges", viewActionShowEdges->isChecked());
  this->qglview->setShowEdges(viewActionShowEdges->isChecked());
  this->qglview->update();
}

void MainWindow::viewModeShowAxes()
{
  const bool showaxes = viewActionShowAxes->isChecked();
  QSettingsCached settings;
  settings.setValue("view/showAxes", showaxes);
  this->viewActionShowScaleProportional->setEnabled(showaxes);
  this->qglview->setShowAxes(showaxes);
  this->qglview->update();
}

void MainWindow::viewModeShowCrosshairs()
{
  QSettingsCached settings;
  settings.setValue("view/showCrosshairs", viewActionShowCrosshairs->isChecked());
  this->qglview->setShowCrosshairs(viewActionShowCrosshairs->isChecked());
  this->qglview->update();
}

void MainWindow::viewModeShowScaleProportional()
{
  QSettingsCached settings;
  settings.setValue("view/showScaleProportional", viewActionShowScaleProportional->isChecked());
  this->qglview->setShowScaleProportional(viewActionShowScaleProportional->isChecked());
  this->qglview->update();
}

bool MainWindow::isEmpty() { return activeEditor->toPlainText().isEmpty(); }

void MainWindow::editorContentChanged()
{
  // this slot is called when the content of the active editor changed.
  // it rely on the activeEditor member to pick the new data.

  auto current_doc = activeEditor->toPlainText();
  if (current_doc != lastCompiledDoc) {
    animateWidget->editorContentChanged();

    // removes the live selection feedbacks in both the 3d view and editor.
    clearAllSelectionIndicators();
  }
}

void MainWindow::viewAngleTop()
{
  qglview->cam.object_rot << 90, 0, 0;
  this->qglview->update();
}

void MainWindow::viewAngleBottom()
{
  qglview->cam.object_rot << 270, 0, 0;
  this->qglview->update();
}

void MainWindow::viewAngleLeft()
{
  qglview->cam.object_rot << 0, 0, 90;
  this->qglview->update();
}

void MainWindow::viewAngleRight()
{
  qglview->cam.object_rot << 0, 0, 270;
  this->qglview->update();
}

void MainWindow::viewAngleFront()
{
  qglview->cam.object_rot << 0, 0, 0;
  this->qglview->update();
}

void MainWindow::viewAngleBack()
{
  qglview->cam.object_rot << 0, 0, 180;
  this->qglview->update();
}

void MainWindow::viewAngleDiagonal()
{
  qglview->cam.object_rot << 35, 0, -25;
  this->qglview->update();
}

void MainWindow::viewCenter()
{
  qglview->cam.object_trans << 0, 0, 0;
  this->qglview->update();
}

void MainWindow::setProjectionType(ProjectionType mode)
{
  bool isOrthogonal = ProjectionType::ORTHOGONAL == mode;
  QSettingsCached settings;
  settings.setValue("view/orthogonalProjection", isOrthogonal);
  viewActionPerspective->setChecked(!isOrthogonal);
  viewActionOrthogonal->setChecked(isOrthogonal);
  qglview->setOrthoMode(isOrthogonal);
  qglview->update();
}

void MainWindow::viewPerspective() { setProjectionType(ProjectionType::PERSPECTIVE); }

void MainWindow::viewOrthogonal() { setProjectionType(ProjectionType::ORTHOGONAL); }

void MainWindow::viewTogglePerspective()
{
  const QSettingsCached settings;
  bool isOrtho = settings.value("view/orthogonalProjection").toBool();
  setProjectionType(isOrtho ? ProjectionType::PERSPECTIVE : ProjectionType::ORTHOGONAL);
}

void MainWindow::viewResetView()
{
  this->qglview->resetView();
  this->qglview->update();
}

void MainWindow::viewAll()
{
  this->qglview->viewAll();
  this->qglview->update();
}

void MainWindow::hideEditorToolbar()
{
  QSettingsCached settings;
  const bool shouldHide = viewActionHideEditorToolBar->isChecked();
  settings.setValue("view/hideEditorToolbar", shouldHide);

  if (shouldHide) {
    editortoolbar->hide();
  } else {
    editortoolbar->show();
  }
}

void MainWindow::hide3DViewToolbar()
{
  QSettingsCached settings;
  const bool shouldHide = viewActionHide3DViewToolBar->isChecked();
  settings.setValue("view/hide3DViewToolbar", shouldHide);

  if (shouldHide) {
    viewerToolBar->hide();
  } else {
    viewerToolBar->show();
  }
}

void MainWindow::showLink(const QString& link)
{
  if (link == "#console") {
    consoleDock->show();
  } else if (link == "#errorlog") {
    errorLogDock->show();
  } else if (link == "#colorlist") {
    colorListDock->show();
  }
}

void MainWindow::onEditorDockVisibilityChanged(bool isVisible)
{
  auto e = (ScintillaEditor *)this->activeEditor;
  if (isVisible) {
    e->qsci->setReadOnly(false);
    e->setupAutoComplete(false);
    editorDock->raise();
    tabManager->setFocus();
  } else {
    // Workaround manually disabling interactions with editor by setting it
    // to read-only when not being shown.  This is an upstream bug from Qt
    // (tracking ticket: https://bugreports.qt.io/browse/QTBUG-82939) and
    // may eventually get resolved at which point this bit and the stuff in
    // the else should be removed. Currently known to affect 5.14.1 and 5.15.0
    e->qsci->setReadOnly(true);
    e->setupAutoComplete(true);
  }
  updateExportActions();
}

void MainWindow::onConsoleDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    frameCompileResult->hide();
    consoleDock->raise();
    console->setFocus();
  }
}

void MainWindow::onErrorLogDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    frameCompileResult->hide();
    errorLogDock->raise();
    errorLogWidget->logTable->setFocus();
  }
}

void MainWindow::onAnimateDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    animateDock->raise();
    animateWidget->setFocus();
  }
}

void MainWindow::onFontListDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    fontListWidget->update_font_list();
    fontListWidget->setFocus();
    fontListDock->raise();
  }
}

void MainWindow::onColorListDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    colorListWidget->setFocus();
    colorListDock->raise();
  }
}

void MainWindow::onViewportControlDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    viewportControlDock->raise();
    viewportControlWidget->setFocus();
  }
}

void MainWindow::onParametersDockVisibilityChanged(bool isVisible)
{
  if (isVisible) {
    parameterDock->raise();
    activeEditor->parameterWidget->scrollArea->setFocus();
  }
}

void MainWindow::onColorListColorSelected(const QString& selectedColor)
{
  activeEditor->insertOrReplaceText(selectedColor);
}

// Use the sender's to detect if we are moving forward/backward in docks
// and search for the next dock to "activate" or "emphasize"
// If no dock can be found, returns the first one.
Dock *MainWindow::getNextDockFromSender(QObject *sender)
{
  int direction = 0;

  auto *action = qobject_cast<QAction *>(sender);
  if (action != nullptr) {
    direction = (action == windowActionNextWindow) ? 1 : -1;
  } else {
    auto *shortcut = qobject_cast<QShortcut *>(sender);
    direction = (shortcut == shortcutNextWindow) ? 1 : -1;
  }

  return findVisibleDockToActivate(direction);
}

void MainWindow::onWindowActionNextPrevHovered()
{
  auto dock = getNextDockFromSender(sender());

  // This can happens if there is no visible dock at all
  if (dock == nullptr) return;

  // Hover signal is emitted at each mouse move, to avoid excessive
  // load we only raise/emphasize if it is not yet done.
  if (rubberBandManager.isEmphasized(dock)) return;

  dock->raise();
  rubberBandManager.emphasize(dock);
}

void MainWindow::onWindowActionNextPrevTriggered()
{
  auto dock = getNextDockFromSender(sender());

  // This can happens if there is no visible dock at all
  if (dock == nullptr) return;

  activateDock(dock);
}

void MainWindow::onWindowShortcutNextPrevActivated()
{
  auto dock = getNextDockFromSender(sender());

  // This can happens if there is no visible dock at all
  if (dock == nullptr) return;

  activateDock(dock);
  rubberBandManager.emphasize(dock);
}

QAction *MainWindow::formatIdentifierToAction(const std::string& identifier) const
{
  FileFormat format;
  if (fileformat::fromIdentifier(identifier, format)) {
    const auto it = exportMap.find(format);
    if (it != exportMap.end()) {
      return it->second;
    }
  }
  return nullptr;
}

void MainWindow::onWindowShortcutExport3DActivated()
{
  QAction *action = formatIdentifierToAction(Settings::Settings::toolbarExport3D.value());
  if (action) {
    action->trigger();
  }
}

void MainWindow::on_editActionInsertTemplate_triggered() { activeEditor->displayTemplates(); }

void MainWindow::on_editActionFoldAll_triggered() { activeEditor->foldUnfold(); }

QString MainWindow::getCurrentFileName() const
{
  if (activeEditor == nullptr) return {};

  const QFileInfo fileInfo(activeEditor->filepath);
  QString fname = _("Untitled.scad");
  if (!fileInfo.fileName().isEmpty()) fname = fileInfo.fileName();
  return fname.replace("&", "&&");
}

void MainWindow::onTabManagerAboutToCloseEditor(EditorInterface *closingEditor)
{
  // This slots is in charge of closing properly the preview when the
  // associated editor is about to close.
  if (closingEditor == renderedEditor) {
    renderedEditor = nullptr;

    // Invalidate renderers before we kill the CSG tree
    this->qglview->setRenderer(nullptr);
#ifdef ENABLE_OPENCSG
    this->previewRenderer = nullptr;
#endif
    this->thrownTogetherRenderer = nullptr;

    // Remove previous CSG tree
    this->absoluteRootNode.reset();

    this->csgRoot.reset();
    this->normalizedRoot.reset();
    this->rootProduct.reset();

    this->rootNode.reset();
    this->tree.setRoot(nullptr);
    this->qglview->update();
  }
}

void MainWindow::onTabManagerEditorContentReloaded(EditorInterface *reloadedEditor)
{
  try {
    // when a new editor is created, it is important to compile the initial geometry
    // so the customizer panels are ok.
    parseDocument(reloadedEditor);
  } catch (const HardWarningException&) {
    exceptionCleanup();
  } catch (const std::exception& ex) {
    UnknownExceptionCleanup(ex.what());
  } catch (...) {
    UnknownExceptionCleanup();
  }

  // updates the content of the Recents Files menu to integrate the one possibly
  // associated with the created editor. The reason is that an editor can be created
  // or updated without a file associated with it.
  updateRecentFileActions();
}

void MainWindow::onTabManagerEditorChanged(EditorInterface *newEditor)
{
  activeEditor = newEditor;

  if (newEditor == nullptr) return;

  parameterDock->setWidget(newEditor->parameterWidget);
  editActionUndo->setEnabled(newEditor->canUndo());

  const QString name = getCurrentFileName();
  setWindowTitle(name);

  consoleDock->setNameSuffix(name);
  errorLogDock->setNameSuffix(name);
  animateDock->setNameSuffix(name);
  fontListDock->setNameSuffix(name);
  colorListDock->setNameSuffix(name);
  viewportControlDock->setNameSuffix(name);

  // If there is no renderedEditor we request for a new preview if the
  // auto-reload is enabled.
  if (renderedEditor == nullptr && designActionAutoReload->isChecked()) {
    actionRenderPreview();
  }
}

Dock *MainWindow::findVisibleDockToActivate(int offset) const
{
  const unsigned int dockCount = docks.size();

  int focusedDockIndice = -1;

  // search among the docks the one that is having the focus. This is done by
  // traversing the widget hierarchy from the focused widget up to the docks that
  // contains it.
  const auto focusWidget = QApplication::focusWidget();
  for (auto widget = focusWidget; widget != nullptr; widget = widget->parentWidget()) {
    for (unsigned int index = 0; index < dockCount; ++index) {
      auto dock = std::get<0>(docks[index]);
      if (dock == focusWidget) {
        focusedDockIndice = index;
      }
    }
  }

  if (focusedDockIndice < 0) {
    focusedDockIndice = 0;
  }

  for (size_t o = 1; o < dockCount; ++o) {
    // starting from dockCount + focusedDockIndice move left or right (o*offset)
    // to find the first visible one. dockCount is there so there is no situation in which
    // (-1) % dockCount
    const int target = (dockCount + focusedDockIndice + o * offset) % dockCount;
    const auto& dock = std::get<0>(docks.at(target));

    if (dock->isVisible()) {
      return dock;
    }
  }
  return nullptr;
}

void MainWindow::activateDock(Dock *dock)
{
  if (dock == nullptr) return;

  // We always need to activate the window.
  if (dock->isFloating()) dock->activateWindow();
  else QMainWindow::activateWindow();

  dock->raise();
  dock->setFocus();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent *event)
{
  setCurrentOutput();
  const QList<QUrl> urls = event->mimeData()->urls();
  for (const auto& url : urls) {
    handleFileDrop(url);
  }
  clearCurrentOutput();
}

void MainWindow::handleFileDrop(const QUrl& url)
{
  if (url.scheme() != "file") return;
  const auto fileName = url.toLocalFile();
  const auto fileInfo = QFileInfo{fileName};
  const auto suffix = fileInfo.suffix().toLower();
  const auto cmd = Importer::knownFileExtensions[suffix];
  if (cmd.isEmpty()) {
    tabManager->open(fileName);
  } else {
    activeEditor->insert(cmd.arg(fileName));
  }
}

void MainWindow::helpAbout()
{
  qApp->setWindowIcon(QApplication::windowIcon());
  auto dialog = new AboutDialog(this);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::helpHomepage() { UIUtils::openHomepageURL(); }

void MainWindow::helpManual() { UIUtils::openUserManualURL(); }

void MainWindow::helpOfflineManual() { UIUtils::openOfflineUserManual(); }

void MainWindow::helpCheatSheet() { UIUtils::openCheatSheetURL(); }

void MainWindow::helpOfflineCheatSheet() { UIUtils::openOfflineCheatSheet(); }

void MainWindow::helpLibrary()
{
  if (!this->libraryInfoDialog) {
    const QString rendererInfo(qglview->getRendererInfo().c_str());
    auto dialog = new LibraryInfoDialog(rendererInfo);
    this->libraryInfoDialog = dialog;
  }
  this->libraryInfoDialog->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (tabManager->shouldClose()) {
    isClosing = true;
    progress_report_fin();
    // Disable invokeMethod calls for consoleOutput during shutdown,
    // otherwise will segfault if echos are in progress.
    hideCurrentOutput();

    QSettingsCached settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    if (this->tempFile) {
      delete this->tempFile;
      this->tempFile = nullptr;
    }
    for (auto dock : findChildren<Dock *>()) {
      dock->disableSettingsUpdate();
    }
    event->accept();
  } else {
    event->ignore();
  }
}

void MainWindow::preferences()
{
  GlobalPreferences::inst()->update();
  GlobalPreferences::inst()->show();
  GlobalPreferences::inst()->activateWindow();
  GlobalPreferences::inst()->raise();
}

void MainWindow::setColorScheme(const QString& scheme)
{
  RenderSettings::inst()->colorscheme = scheme.toStdString();
  this->qglview->setColorScheme(scheme.toStdString());
  this->qglview->update();
}

void MainWindow::setFont(const QString& family, uint size)
{
  QFont font;
  if (!family.isEmpty()) font.setFamily(family);
  else font.setFixedPitch(true);
  if (size > 0) font.setPointSize(size);
  font.setStyleHint(QFont::TypeWriter);
  activeEditor->setFont(font);
}

void MainWindow::consoleOutput(const Message& msgObj, void *userdata)
{
  // Invoke the method in the main thread in case the output
  // originates in a worker thread.
  auto thisp = static_cast<MainWindow *>(userdata);
  QMetaObject::invokeMethod(thisp, "consoleOutput", Q_ARG(Message, msgObj));
}

void MainWindow::consoleOutput(const Message& msgObj)
{
  this->console->addMessage(msgObj);
  if (msgObj.group == message_group::Warning || msgObj.group == message_group::Deprecated) {
    ++this->compileWarnings;
  } else if (msgObj.group == message_group::Error) {
    ++this->compileErrors;
  }
  // FIXME: scad parsing/evaluation should be done on separate thread so as not to block the gui.
  // Then processEvents should no longer be needed here.
  this->processEvents();
  if (consoleUpdater && !consoleUpdater->isActive()) {
    consoleUpdater->start(50);  // Limit console updates to 20 FPS
  }
}

void MainWindow::consoleOutputRaw(const QString& html)
{
  this->console->addHtml(html);
  this->processEvents();
}

void MainWindow::errorLogOutput(const Message& log_msg, void *userdata)
{
  auto thisp = static_cast<MainWindow *>(userdata);
  QMetaObject::invokeMethod(thisp, "errorLogOutput", Q_ARG(Message, log_msg));
}

void MainWindow::errorLogOutput(const Message& log_msg) { this->errorLogWidget->toErrorLog(log_msg); }

void MainWindow::setCurrentOutput()
{
  set_output_handler(&MainWindow::consoleOutput, &MainWindow::errorLogOutput, this);
}

void MainWindow::hideCurrentOutput()
{
  set_output_handler(&MainWindow::noOutputConsole, &MainWindow::noOutputErrorLog, this);
}

void MainWindow::clearCurrentOutput() { set_output_handler(nullptr, nullptr, nullptr); }

void MainWindow::openCSGSettingsChanged()
{
#ifdef ENABLE_OPENCSG
  OpenCSG::setOption(OpenCSG::AlgorithmSetting,
                     GlobalPreferences::inst()->getValue("advanced/forceGoldfeather").toBool()
                       ? OpenCSG::Goldfeather
                       : OpenCSG::Automatic);
#endif
}

void MainWindow::processEvents()
{
  if (this->procevents) QApplication::processEvents();
}

QString MainWindow::exportPath(const QString& suffix)
{
  const auto path_it = this->exportPaths.find(suffix);
  const auto basename =
    activeEditor->filepath.isEmpty() ? "Untitled" : QFileInfo(activeEditor->filepath).completeBaseName();
  QString dir;
  if (path_it != exportPaths.end()) {
    dir = QFileInfo(path_it->second).absolutePath();
  } else if (activeEditor->filepath.isEmpty()) {
    dir = QString::fromStdString(PlatformUtils::userDocumentsPath());
  } else {
    dir = QFileInfo(activeEditor->filepath).absolutePath();
  }
  return QString("%1/%2.%3").arg(dir, basename, suffix);
}

void MainWindow::jumpToLine(int line, int col) { this->activeEditor->setCursorPosition(line, col); }

void MainWindow::resetMeasurementsState(bool enable, const QString& tooltipMessage)
{
  if (RenderSettings::inst()->backend3D != RenderBackend3D::ManifoldBackend) {
    enable = false;
    static const auto noCGALMessage =
      "Measurements only work with Manifold backend; Preferences->Advanced->3D Rendering->Backend";
    this->designActionMeasureDist->setToolTip(noCGALMessage);
    this->designActionMeasureAngle->setToolTip(noCGALMessage);
  } else {
    this->designActionMeasureDist->setToolTip(tooltipMessage);
    this->designActionMeasureAngle->setToolTip(tooltipMessage);
  }

  this->designActionMeasureDist->setEnabled(enable);
  this->designActionMeasureDist->setChecked(false);
  this->designActionMeasureAngle->setEnabled(enable);
  this->designActionMeasureAngle->setChecked(false);

  (void)meas.stopMeasure();
  activeMeasurement = nullptr;
}
