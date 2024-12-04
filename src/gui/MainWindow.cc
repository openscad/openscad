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

#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QMetaObject>
#include <QPoint>
#include <QSoundEffect>
#include <QStringList>
#include <QTextEdit>
#include <QToolBar>
#include <QWidget>
#include <deque>
#include <cassert>
#include <array>
#include <functional>
#include <exception>
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/manifoldutils.h"
#endif
#include "core/Builtins.h"
#include "core/BuiltinContext.h"
#include "core/customizer/CommentParser.h"
#include "core/RenderVariables.h"
#include "openscad.h"
#include "geometry/GeometryCache.h"
#include "core/SourceFileCache.h"
#include "gui/OpenSCADApp.h"
#include "core/parsersettings.h"
#include "glview/RenderSettings.h"
#include "gui/Preferences.h"
#include "utils/printutils.h"
#include "core/node.h"
#include "core/CSGNode.h"
#include "core/Expression.h"
#include "core/ScopeContext.h"
#include "core/progress.h"
#include "io/dxfdim.h"
#include "io/fileutils.h"
#include "gui/Settings.h"
#include "gui/AboutDialog.h"
#include "gui/FontListDialog.h"
#include "gui/LibraryInfoDialog.h"
#include "gui/ScintillaEditor.h"
#ifdef ENABLE_OPENCSG
#include "core/CSGTreeEvaluator.h"
#include "glview/preview/OpenCSGRenderer.h"
#ifdef USE_LEGACY_RENDERERS
#include "glview/preview/LegacyOpenCSGRenderer.h"
#endif
#include <opencsg.h>
#endif
#include "gui/ProgressWidget.h"
#include "glview/preview/ThrownTogetherRenderer.h"
#ifdef USE_LEGACY_RENDERERS
#include "glview/preview/LegacyThrownTogetherRenderer.h"
#endif
#include "glview/preview/CSGTreeNormalizer.h"
#include "gui/QGLView.h"
#include "gui/MouseSelector.h"
#ifdef Q_OS_MACOS
#include "platform/CocoaUtils.h"
#endif
#ifdef Q_OS_WIN
#include <QScreen>
#endif
#include "platform/PlatformUtils.h"
#ifdef OPENSCAD_UPDATER
#include "gui/AutoUpdater.h"
#endif
#include "gui/TabManager.h"
#include "gui/ExternalToolInterface.h"

#include <QMenu>
#include <QTime>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QTextStream>
#include <QStatusBar>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProgressDialog>
#include <QMutexLocker>
#include <QTemporaryFile>
#include <QDockWidget>
#include <QClipboard>
#include <QProcess>
#include <memory>
#include <string>
#include "gui/QWordSearchField.h"
#include <QSettings> //Include QSettings for direct operations on settings arrays
#include "gui/QSettingsCached.h"

#ifdef ENABLE_PYTHON
extern std::shared_ptr<AbstractNode> python_result_node;
std::string evaluatePython(const std::string& code, double time);
extern bool python_trusted;

#include "cryptopp/sha.h"
#include "cryptopp/filters.h"
#include "cryptopp/base64.h"

std::string SHA256HashString(std::string aString){
  std::string digest;
  CryptoPP::SHA256 hash;

  CryptoPP::StringSource foo(aString, true,
                             new CryptoPP::HashFilter(hash,
                                                      new CryptoPP::Base64Encoder(
                                                        new CryptoPP::StringSink(digest))));

  return digest;
}

#endif // ifdef ENABLE_PYTHON

#define ENABLE_3D_PRINTING
#include "gui/OctoPrint.h"
#include "gui/PrintService.h"

#include <fstream>

#include <algorithm>
#include <boost/version.hpp>
#include <sys/stat.h>

#include "glview/cgal/CGALRenderer.h"
#include "glview/cgal/LegacyCGALRenderer.h"
#include "gui/CGALWorker.h"

#ifdef ENABLE_CGAL
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "geometry/cgal/CGALCache.h"
#include "geometry/cgal/CGAL_Nef_polyhedron.h"
#include "geometry/cgal/CGALHybridPolyhedron.h"
#endif // ENABLE_CGAL

#ifdef ENABLE_MANIFOLD
#include "geometry/manifold/ManifoldGeometry.h"
#endif // ENABLE_MANIFOLD

#include "geometry/GeometryEvaluator.h"

#include "gui/PrintInitDialog.h"
//#include "gui/ExportPdfDialog.h"
#include "gui/input/InputDriverEvent.h"
#include "gui/input/InputDriverManager.h"
#include <cstdio>
#include <memory>
#include <QtNetwork>
#include <utility>

#include "gui/qt-obsolete.h" // IWYU pragma: keep

static const int autoReloadPollingPeriodMS = 200;

// Global application state
unsigned int GuiLocker::gui_locked = 0;

static char copyrighttext[] =
  "<p>Copyright (C) 2009-2024 The OpenSCAD Developers</p>"
  "<p>This program is free software; you can redistribute it and/or modify "
  "it under the terms of the GNU General Public License as published by "
  "the Free Software Foundation; either version 2 of the License, or "
  "(at your option) any later version.<p>";
bool MainWindow::undockMode = false;
bool MainWindow::reorderMode = false;
const int MainWindow::tabStopWidth = 15;
QElapsedTimer *MainWindow::progressThrottle = new QElapsedTimer();

namespace {

struct DockFocus {
  QWidget *widget;
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

void fileExportedMessage(const char *format, const QString& filename) {
  LOG("%1$s export finished: %2$s", format, filename.toUtf8().constData());
}

void removeExportActions(QToolBar *toolbar, QAction *action) {
  int idx = toolbar->actions().indexOf(action);
  while (idx > 0) {
    QAction *a = toolbar->actions().at(idx - 1);
    if (a->objectName().isEmpty()) // separator
      break;
    toolbar->removeAction(a);
    idx--;
  }
}

void addExportActions(const MainWindow *mainWindow, QToolBar *toolbar, QAction *action) {
  for (const std::string &identifier : {Settings::Settings::toolbarExport3D.value(), 
                                        Settings::Settings::toolbarExport2D.value()}) {
    FileFormat format;
    fileformat::fromIdentifier(identifier, format);
    const auto it=mainWindow->export_map.find(format);
    // FIXME: Allow turning off the toolbar entry?
    if (it != mainWindow->export_map.end()) {
      toolbar->insertAction(action, it->second);
    }
  }
}

} // namespace

MainWindow::MainWindow(const QStringList& filenames)
{
  setupUi(this);

  consoleUpdater = new QTimer(this);
  consoleUpdater->setSingleShot(true);
  connect(consoleUpdater, SIGNAL(timeout()), this->console, SLOT(update()));

  editorDockTitleWidget = new QWidget();
  consoleDockTitleWidget = new QWidget();
  parameterDockTitleWidget = new QWidget();
  errorLogDockTitleWidget = new QWidget();
  animateDockTitleWidget = new QWidget();
  fontListDockTitleWidget = new QWidget();
  viewportControlTitleWidget = new QWidget();

  this->animateWidget->setMainWindow(this);
  this->viewportControlWidget->setMainWindow(this);
  // actions not included in menu
  this->addAction(editActionInsertTemplate);
  this->addAction(editActionFoldAll);

  this->editorDock->setConfigKey("view/hideEditor");
  this->editorDock->setAction(this->windowActionHideEditor);
  this->consoleDock->setConfigKey("view/hideConsole");
  this->consoleDock->setAction(this->windowActionHideConsole);
  this->parameterDock->setConfigKey("view/hideCustomizer");
  this->parameterDock->setAction(this->windowActionHideCustomizer);
  this->errorLogDock->setConfigKey("view/hideErrorLog");
  this->errorLogDock->setAction(this->windowActionHideErrorLog);
  this->animateDock->setConfigKey("view/hideAnimate");
  this->animateDock->setAction(this->windowActionHideAnimate);
  this->fontListDock->setConfigKey("view/hideFontList");
  this->fontListDock->setAction(this->windowActionHideFontList);
  this->viewportControlDock->setConfigKey("view/hideViewportControl");
  this->viewportControlDock->setAction(this->windowActionHideViewportControl);

  this->versionLabel = nullptr; // must be initialized before calling updateStatusBar()
  updateStatusBar(nullptr);

  renderCompleteSoundEffect = new QSoundEffect();
  renderCompleteSoundEffect->setSource(QUrl("qrc:/sounds/complete.wav"));

  const QString importStatement = "import(\"%1\");\n";
  const QString surfaceStatement = "surface(\"%1\");\n";
  const QString importFunction = "data = import(\"%1\");\n";
  knownFileExtensions["stl"] = importStatement;
  knownFileExtensions["obj"] = importStatement;
  knownFileExtensions["3mf"] = importStatement;
  knownFileExtensions["off"] = importStatement;
  knownFileExtensions["dxf"] = importStatement;
  knownFileExtensions["svg"] = importStatement;
  knownFileExtensions["amf"] = importStatement;
  knownFileExtensions["dat"] = surfaceStatement;
  knownFileExtensions["png"] = surfaceStatement;
  knownFileExtensions["json"] = importFunction;
  knownFileExtensions["scad"] = "";
#ifdef ENABLE_PYTHON
  knownFileExtensions["py"] = "";
#endif
  knownFileExtensions["csg"] = "";

  root_file = nullptr;
  parsed_file = nullptr;
  absolute_root_node = nullptr;

  // Open Recent
  for (auto& recent : this->actionRecentFile) {
    recent = new QAction(this);
    recent->setVisible(false);
    this->menuOpenRecent->addAction(recent);
    connect(recent, SIGNAL(triggered()),
            this, SLOT(actionOpenRecent()));
  }

  // Preferences initialization happens on first tab creation, and depends on colorschemes from editor.
  // Any code dependent on Preferences must come after the TabManager instantiation
  tabManager = new TabManager(this, filenames.isEmpty() ? QString() : filenames[0]);
  connect(tabManager, SIGNAL(tabCountChanged(int)), this, SLOT(setTabToolBarVisible(int)));
  this->setTabToolBarVisible(tabManager->count());
  tabToolBarContents->layout()->addWidget(tabManager->getTabHeader());
  editorDockContents->layout()->addWidget(tabManager->getTabContent());

  connect(Preferences::inst(), SIGNAL(consoleFontChanged(const QString&,uint)), this->console, SLOT(setFont(const QString&,uint)));

  const QString version = QString("<b>OpenSCAD %1</b>").arg(QString::fromStdString(openscad_versionnumber));
  const QString weblink = "<a href=\"https://www.openscad.org/\">https://www.openscad.org/</a><br>";
  this->console->setFont(
    Preferences::inst()->getValue("advanced/consoleFontFamily").toString(),
    Preferences::inst()->getValue("advanced/consoleFontSize").toUInt()
    );

  consoleOutputRaw(version);
  consoleOutputRaw(weblink);
  consoleOutputRaw(copyrighttext);
  this->consoleUpdater->start(0); // Show "Loaded Design" message from TabManager

  connect(this->errorLogWidget, SIGNAL(openFile(QString,int)), this, SLOT(openFileFromPath(QString,int)));
  connect(this->console, SIGNAL(openFile(QString,int)), this, SLOT(openFileFromPath(QString,int)));

  connect(Preferences::inst()->ButtonConfig, SIGNAL(inputMappingChanged()), InputDriverManager::instance(), SLOT(onInputMappingUpdated()), Qt::UniqueConnection);
  connect(Preferences::inst()->AxisConfig, SIGNAL(inputMappingChanged()), InputDriverManager::instance(), SLOT(onInputMappingUpdated()), Qt::UniqueConnection);
  connect(Preferences::inst()->AxisConfig, SIGNAL(inputCalibrationChanged()), InputDriverManager::instance(), SLOT(onInputCalibrationUpdated()), Qt::UniqueConnection);
  connect(Preferences::inst()->AxisConfig, SIGNAL(inputGainChanged()), InputDriverManager::instance(), SLOT(onInputGainUpdated()), Qt::UniqueConnection);

  setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->setAttribute(Qt::WA_DeleteOnClose);

  scadApp->windowManager.add(this);

  this->cgalworker = new CGALWorker();
  connect(this->cgalworker, SIGNAL(done(std::shared_ptr<const Geometry>)),
          this, SLOT(actionRenderDone(std::shared_ptr<const Geometry>)));

  root_node = nullptr;

  this->qglview->statusLabel = new QLabel(this);
  this->qglview->statusLabel->setMinimumWidth(100);
  statusBar()->addWidget(this->qglview->statusLabel);

  QSettingsCached settings;
  this->qglview->setMouseCentricZoom(Settings::Settings::mouseCentricZoom.value());
  this->qglview->setMouseSwapButtons(Settings::Settings::mouseSwapButtons.value());
  this->meas.setView(qglview);
  this->designActionMeasureDist->setEnabled(false);
  this->designActionMeasureAngle->setEnabled(false);

  autoReloadTimer = new QTimer(this);
  autoReloadTimer->setSingleShot(false);
  autoReloadTimer->setInterval(autoReloadPollingPeriodMS);
  connect(autoReloadTimer, SIGNAL(timeout()), this, SLOT(checkAutoReload()));

  this->exportformat_mapper = new QSignalMapper(this);
  connect(this->exportformat_mapper, SIGNAL(mappedInt(int)), this, SLOT(actionExportFileFormat(int))) ;

  waitAfterReloadTimer = new QTimer(this);
  waitAfterReloadTimer->setSingleShot(true);
  waitAfterReloadTimer->setInterval(autoReloadPollingPeriodMS);
  connect(waitAfterReloadTimer, SIGNAL(timeout()), this, SLOT(waitAfterReload()));
  connect(Preferences::inst(), SIGNAL(ExperimentalChanged()), this, SLOT(changeParameterWidget()));

  progressThrottle->start();

  this->hideFind();
  frameCompileResult->hide();
  this->labelCompileResultMessage->setOpenExternalLinks(false);
  connect(this->labelCompileResultMessage, SIGNAL(linkActivated(QString)), SLOT(showLink(QString)));

  // File menu
  connect(this->fileActionNewWindow, SIGNAL(triggered()), this, SLOT(actionNewWindow()));
  connect(this->fileActionNew, SIGNAL(triggered()), tabManager, SLOT(actionNew()));
  connect(this->fileActionOpenWindow, SIGNAL(triggered()), this, SLOT(actionOpenWindow()));
  connect(this->fileActionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
  connect(this->fileActionSave, SIGNAL(triggered()), this, SLOT(actionSave()));
  connect(this->fileActionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs()));
  connect(this->fileActionSaveACopy, SIGNAL(triggered()), this, SLOT(actionSaveACopy()));
  connect(this->fileActionSaveAll, SIGNAL(triggered()), tabManager, SLOT(saveAll()));
  connect(this->fileActionReload, SIGNAL(triggered()), this, SLOT(actionReload()));
  connect(this->fileActionRevoke, SIGNAL(triggered()), this, SLOT(actionRevokeTrustedFiles()));
  connect(this->fileActionClose, SIGNAL(triggered()), tabManager, SLOT(closeCurrentTab()));
  connect(this->fileActionQuit, SIGNAL(triggered()), this, SLOT(quit()));
  connect(this->fileShowLibraryFolder, SIGNAL(triggered()), this, SLOT(actionShowLibraryFolder()));
#ifndef __APPLE__
  auto shortcuts = this->fileActionSave->shortcuts();
  this->fileActionSave->setShortcuts(shortcuts);
  shortcuts = this->fileActionReload->shortcuts();
  shortcuts.push_back(QKeySequence(Qt::Key_F3));
  this->fileActionReload->setShortcuts(shortcuts);
#endif

  this->menuOpenRecent->addSeparator();
  this->menuOpenRecent->addAction(this->fileActionClearRecent);
  connect(this->fileActionClearRecent, SIGNAL(triggered()),
          this, SLOT(clearRecentFiles()));

  show_examples();

  connect(this->editActionNextTab, SIGNAL(triggered()), tabManager, SLOT(nextTab()));
  connect(this->editActionPrevTab, SIGNAL(triggered()), tabManager, SLOT(prevTab()));

  connect(this->editActionCopy, SIGNAL(triggered()), this, SLOT(copyText()));
  connect(this->editActionCopyViewport, SIGNAL(triggered()), this, SLOT(actionCopyViewport()));
  connect(this->editActionConvertTabsToSpaces, SIGNAL(triggered()), this, SLOT(convertTabsToSpaces()));
  connect(this->editActionCopyVPT, SIGNAL(triggered()), this, SLOT(copyViewportTranslation()));
  connect(this->editActionCopyVPR, SIGNAL(triggered()), this, SLOT(copyViewportRotation()));
  connect(this->editActionCopyVPD, SIGNAL(triggered()), this, SLOT(copyViewportDistance()));
  connect(this->editActionCopyVPF, SIGNAL(triggered()), this, SLOT(copyViewportFov()));
  connect(this->editActionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));
  // Edit->Find
  connect(this->editActionFind, SIGNAL(triggered()), this, SLOT(showFind()));
  connect(this->editActionFindAndReplace, SIGNAL(triggered()), this, SLOT(showFindAndReplace()));
#ifdef Q_OS_WIN
  this->editActionFindAndReplace->setShortcut(QKeySequence(Qt::CTRL, Qt::SHIFT, Qt::Key_F));
#endif
  connect(this->editActionFindNext, SIGNAL(triggered()), this, SLOT(findNext()));
  connect(this->editActionFindPrevious, SIGNAL(triggered()), this, SLOT(findPrev()));
  connect(this->editActionUseSelectionForFind, SIGNAL(triggered()), this, SLOT(useSelectionForFind()));

  // Design menu
  connect(this->designActionAutoReload, SIGNAL(toggled(bool)), this, SLOT(autoReloadSet(bool)));
  connect(this->designActionReloadAndPreview, SIGNAL(triggered()), this, SLOT(actionReloadRenderPreview()));
  connect(this->designActionPreview, SIGNAL(triggered()), this, SLOT(actionRenderPreview()));
  connect(this->designActionRender, SIGNAL(triggered()), this, SLOT(actionRender()));
  connect(this->designActionMeasureDist, SIGNAL(triggered()), this, SLOT(actionMeasureDistance()));
  connect(this->designActionMeasureAngle, SIGNAL(triggered()), this, SLOT(actionMeasureAngle()));
  connect(this->designAction3DPrint, SIGNAL(triggered()), this, SLOT(action3DPrint()));
  connect(this->designCheckValidity, SIGNAL(triggered()), this, SLOT(actionCheckValidity()));
  connect(this->designActionDisplayAST, SIGNAL(triggered()), this, SLOT(actionDisplayAST()));
  connect(this->designActionDisplayCSGTree, SIGNAL(triggered()), this, SLOT(actionDisplayCSGTree()));
  connect(this->designActionDisplayCSGProducts, SIGNAL(triggered()), this, SLOT(actionDisplayCSGProducts()));

  export_map[FileFormat::BINARY_STL] =this->fileActionExportBinarySTL;
  export_map[FileFormat::ASCII_STL] =this->fileActionExportAsciiSTL;
  export_map[FileFormat::_3MF] = this->fileActionExport3MF;
  export_map[FileFormat::OBJ] =  this->fileActionExportOBJ;
  export_map[FileFormat::OFF] =  this->fileActionExportOFF;
  export_map[FileFormat::WRL] =  this->fileActionExportWRL;
  export_map[FileFormat::POV] =  this->fileActionExportPOV;
  export_map[FileFormat::AMF] =  this->fileActionExportAMF;
  export_map[FileFormat::DXF] =  this->fileActionExportDXF;
  export_map[FileFormat::SVG] =  this->fileActionExportSVG;
  export_map[FileFormat::PDF] =  this->fileActionExportPDF;
  export_map[FileFormat::CSG] =  this->fileActionExportCSG;
  export_map[FileFormat::PNG] =  this->fileActionExportImage;

  for (auto &pair : export_map) {
    connect(pair.second, SIGNAL(triggered()), this->exportformat_mapper, SLOT(map()));
    this->exportformat_mapper->setMapping(pair.second, int(pair.first));
  }

  connect(this->designActionFlushCaches, SIGNAL(triggered()), this, SLOT(actionFlushCaches()));

#ifndef ENABLE_LIB3MF
  this->fileActionExport3MF->setVisible(false);
#endif

#ifndef ENABLE_3D_PRINTING
  this->designAction3DPrint->setVisible(false);
  this->designAction3DPrint->setEnabled(false);
#endif

  // View menu
#ifndef ENABLE_OPENCSG
  this->viewActionPreview->setVisible(false);
#else
  connect(this->viewActionPreview, SIGNAL(triggered()), this, SLOT(viewModePreview()));
  if (!this->qglview->hasOpenCSGSupport()) {
    this->viewActionPreview->setEnabled(false);
  }
#endif

  connect(this->viewActionSurfaces, SIGNAL(triggered()), this, SLOT(viewModeSurface()));
  connect(this->viewActionWireframe, SIGNAL(triggered()), this, SLOT(viewModeWireframe()));
  connect(this->viewActionThrownTogether, SIGNAL(triggered()), this, SLOT(viewModeThrownTogether()));
  connect(this->viewActionShowEdges, SIGNAL(triggered()), this, SLOT(viewModeShowEdges()));
  connect(this->viewActionShowAxes, SIGNAL(triggered()), this, SLOT(viewModeShowAxes()));
  connect(this->viewActionShowCrosshairs, SIGNAL(triggered()), this, SLOT(viewModeShowCrosshairs()));
  connect(this->viewActionShowScaleProportional, SIGNAL(triggered()), this, SLOT(viewModeShowScaleProportional()));
  connect(this->viewActionTop, SIGNAL(triggered()), this, SLOT(viewAngleTop()));
  connect(this->viewActionBottom, SIGNAL(triggered()), this, SLOT(viewAngleBottom()));
  connect(this->viewActionLeft, SIGNAL(triggered()), this, SLOT(viewAngleLeft()));
  connect(this->viewActionRight, SIGNAL(triggered()), this, SLOT(viewAngleRight()));
  connect(this->viewActionFront, SIGNAL(triggered()), this, SLOT(viewAngleFront()));
  connect(this->viewActionBack, SIGNAL(triggered()), this, SLOT(viewAngleBack()));
  connect(this->viewActionDiagonal, SIGNAL(triggered()), this, SLOT(viewAngleDiagonal()));
  connect(this->viewActionCenter, SIGNAL(triggered()), this, SLOT(viewCenter()));
  connect(this->viewActionResetView, SIGNAL(triggered()), this, SLOT(viewResetView()));
  connect(this->viewActionViewAll, SIGNAL(triggered()), this, SLOT(viewAll()));
  connect(this->viewActionPerspective, SIGNAL(triggered()), this, SLOT(viewPerspective()));
  connect(this->viewActionOrthogonal, SIGNAL(triggered()), this, SLOT(viewOrthogonal()));
  connect(this->viewActionZoomIn, SIGNAL(triggered()), qglview, SLOT(ZoomIn()));
  connect(this->viewActionZoomOut, SIGNAL(triggered()), qglview, SLOT(ZoomOut()));
  connect(this->viewActionHideEditorToolBar, SIGNAL(triggered()), this, SLOT(hideEditorToolbar()));
  connect(this->viewActionHide3DViewToolBar, SIGNAL(triggered()), this, SLOT(hide3DViewToolbar()));
  connect(this->windowActionHideEditor, SIGNAL(triggered()), this, SLOT(hideEditor()));
  connect(this->windowActionHideConsole, SIGNAL(triggered()), this, SLOT(hideConsole()));
  connect(this->windowActionHideCustomizer, SIGNAL(triggered()), this, SLOT(hideParameters()));
  connect(this->windowActionHideErrorLog, SIGNAL(triggered()), this, SLOT(hideErrorLog()));
  connect(this->windowActionHideAnimate, SIGNAL(triggered()), this, SLOT(hideAnimate()));
  connect(this->windowActionHideFontList, SIGNAL(triggered()), this, SLOT(hideFontList()));
  connect(this->windowActionHideViewportControl, SIGNAL(triggered()), this, SLOT(hideViewportControl()));

  // Help menu
  connect(this->helpActionAbout, SIGNAL(triggered()), this, SLOT(helpAbout()));
  connect(this->helpActionHomepage, SIGNAL(triggered()), this, SLOT(helpHomepage()));
  connect(this->helpActionManual, SIGNAL(triggered()), this, SLOT(helpManual()));
  connect(this->helpActionCheatSheet, SIGNAL(triggered()), this, SLOT(helpCheatSheet()));
  connect(this->helpActionLibraryInfo, SIGNAL(triggered()), this, SLOT(helpLibrary()));
  connect(this->helpActionFontInfo, SIGNAL(triggered()), this, SLOT(helpFontInfo()));

  // Checks if the Documentation has been downloaded and hides the Action otherwise
  if (UIUtils::hasOfflineUserManual()) {
    connect(this->helpActionOfflineManual, SIGNAL(triggered()), this, SLOT(helpOfflineManual()));
  } else {
    this->helpActionOfflineManual->setVisible(false);
  }
  if (UIUtils::hasOfflineCheatSheet()) {
    connect(this->helpActionOfflineCheatSheet, SIGNAL(triggered()), this, SLOT(helpOfflineCheatSheet()));
  } else {
    this->helpActionOfflineCheatSheet->setVisible(false);
  }
#ifdef OPENSCAD_UPDATER
  this->menuBar()->addMenu(AutoUpdater::updater()->updateMenu);
#endif

  connect(this->qglview, SIGNAL(cameraChanged()), animateWidget, SLOT(cameraChanged()));
  connect(this->qglview, SIGNAL(cameraChanged()), viewportControlWidget, SLOT(cameraChanged()));
  connect(this->qglview, SIGNAL(resized()), viewportControlWidget, SLOT(viewResized()));
  connect(this->qglview, SIGNAL(doRightClick(QPoint)), this, SLOT(rightClick(QPoint)));
  connect(this->qglview, SIGNAL(doLeftClick(QPoint)), this, SLOT(leftClick(QPoint)));

  connect(Preferences::inst(), SIGNAL(requestRedraw()), this->qglview, SLOT(update()));
  connect(Preferences::inst(), SIGNAL(updateMouseCentricZoom(bool)), this->qglview, SLOT(setMouseCentricZoom(bool)));
  connect(Preferences::inst(), SIGNAL(updateMouseSwapButtons(bool)), this->qglview, SLOT(setMouseSwapButtons(bool)));
  connect(Preferences::inst(), SIGNAL(updateReorderMode(bool)), this, SLOT(updateReorderMode(bool)));
  connect(Preferences::inst(), SIGNAL(updateUndockMode(bool)), this, SLOT(updateUndockMode(bool)));
  connect(Preferences::inst(), SIGNAL(openCSGSettingsChanged()), this, SLOT(openCSGSettingsChanged()));
  connect(Preferences::inst(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setColorScheme(const QString&)));
  connect(Preferences::inst(), SIGNAL(toolbarExportChanged()), this, SLOT(updateExportActions()));

  Preferences::inst()->apply_win(); // not sure if to be commented, checked must not be commented(done some changes in apply())

  QString cs = Preferences::inst()->getValue("3dview/colorscheme").toString();
  this->setColorScheme(cs);

  //find and replace panel
  connect(this->findTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFindType(int)));
  connect(this->findInputField, SIGNAL(textChanged(QString)), this, SLOT(findString(QString)));
  connect(this->findInputField, SIGNAL(returnPressed()), this->findNextButton, SLOT(animateClick()));
  find_panel->installEventFilter(this);
  if (QApplication::clipboard()->supportsFindBuffer()) {
    connect(this->findInputField, SIGNAL(textChanged(QString)), this, SLOT(updateFindBuffer(QString)));
    connect(QApplication::clipboard(), SIGNAL(findBufferChanged()), this, SLOT(findBufferChanged()));
    // With Qt 4.8.6, there seems to be a bug that often gives an incorrect findbuffer content when
    // the app receives focus for the first time
    this->findInputField->setText(QApplication::clipboard()->text(QClipboard::FindBuffer));
  }

  connect(this->findPrevButton, SIGNAL(clicked()), this, SLOT(findPrev()));
  connect(this->findNextButton, SIGNAL(clicked()), this, SLOT(findNext()));
  connect(this->cancelButton, SIGNAL(clicked()), this, SLOT(hideFind()));
  connect(this->replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
  connect(this->replaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));
  connect(this->replaceInputField, SIGNAL(returnPressed()), this->replaceButton, SLOT(animateClick()));
  addKeyboardShortCut(this->viewerToolBar->actions());
  addKeyboardShortCut(this->editortoolbar->actions());

  Preferences *instance = Preferences::inst();

  initActionIcon(fileActionNew, ":/icons/svg-default/new.svg", ":/icons/svg-default/new-white.svg");
  initActionIcon(fileActionOpen, ":/icons/svg-default/open.svg", ":/icons/svg-default/open-white.svg");
  initActionIcon(fileActionSave, ":/icons/svg-default/save.svg", ":/icons/svg-default/save-white.svg");
  initActionIcon(editActionZoomTextIn, ":/icons/svg-default/zoom-text-in.svg", ":/icons/svg-default/zoom-text-in-white.svg");
  initActionIcon(editActionZoomTextOut, ":/icons/svg-default/zoom-text-out.svg", ":/icons/svg-default/zoom-text-out-white.svg");
  initActionIcon(designActionRender, ":/icons/svg-default/render.svg", ":/icons/svg-default/render-white.svg");
  initActionIcon(designAction3DPrint, ":/icons/svg-default/send.svg", ":/icons/svg-default/send-white.svg");
  initActionIcon(viewActionShowAxes, ":/icons/svg-default/axes.svg", ":/icons/svg-default/axes-white.svg");
  initActionIcon(viewActionShowEdges, ":/icons/svg-default/show-edges.svg", ":/icons/svg-default/show-edges-white.svg");
  initActionIcon(viewActionZoomIn, ":/icons/svg-default/zoom-in.svg", ":/icons/svg-default/zoom-in-white.svg");
  initActionIcon(viewActionZoomOut, ":/icons/svg-default/zoom-out.svg", ":/icons/svg-default/zoom-out-white.svg");
  initActionIcon(viewActionTop, ":/icons/svg-default/view-top.svg", ":/icons/svg-default/view-top-white.svg");
  initActionIcon(viewActionBottom, ":/icons/svg-default/view-bottom.svg", ":/icons/svg-default/view-bottom-white.svg");
  initActionIcon(viewActionLeft, ":/icons/svg-default/view-left.svg", ":/icons/svg-default/view-left-white.svg");
  initActionIcon(viewActionRight, ":/icons/svg-default/view-right.svg", ":/icons/svg-default/view-right-white.svg");
  initActionIcon(viewActionFront, ":/icons/svg-default/view-front.svg", ":/icons/svg-default/view-front-white.svg");
  initActionIcon(viewActionBack, ":/icons/svg-default/view-back.svg", ":/icons/svg-default/view-back-white.svg");
  initActionIcon(viewActionSurfaces, ":/icons/svg-default/surface.svg", ":/icons/svg-default/surface-white.svg");
  initActionIcon(viewActionWireframe, ":/icons/svg-default/wireframe.svg", ":/icons/svg-default/wireframe-white.svg");
  initActionIcon(viewActionShowCrosshairs, ":/icons/svg-default/crosshairs.svg", ":/icons/svg-default/crosshairs-white.svg");
  initActionIcon(viewActionThrownTogether, ":/icons/svg-default/throwntogether.svg", ":/icons/svg-default/throwntogether-white.svg");
  initActionIcon(viewActionPerspective, ":/icons/svg-default/perspective.svg", ":/icons/svg-default/perspective-white.svg");
  initActionIcon(viewActionOrthogonal, ":/icons/svg-default/orthogonal.svg", ":/icons/svg-default/orthogonal-white.svg");
  initActionIcon(designActionPreview, ":/icons/svg-default/preview.svg", ":/icons/svg-default/preview-white.svg");
  initActionIcon(designActionMeasureDist, ":/icons/svg-default/measure-dist.svg", ":/icons/svg-default/measure-dist-white.svg");
  initActionIcon(designActionMeasureAngle, ":/icons/svg-default/measure-ang.svg", ":/icons/svg-default/measure-ang-white.svg");
  initActionIcon(fileActionExportBinarySTL, ":/icons/svg-default/export-stl.svg", ":/icons/svg-default/export-stl-white.svg");
  initActionIcon(fileActionExportAsciiSTL, ":/icons/svg-default/export-stl.svg", ":/icons/svg-default/export-stl-white.svg");
  initActionIcon(fileActionExportAMF, ":/icons/svg-default/export-amf.svg", ":/icons/svg-default/export-amf-white.svg");
  initActionIcon(fileActionExport3MF, ":/icons/svg-default/export-3mf.svg", ":/icons/svg-default/export-3mf-white.svg");
  initActionIcon(fileActionExportOBJ, ":/icons/svg-default/export-obj.svg", ":/icons/svg-default/export-obj-white.svg");
  initActionIcon(fileActionExportOFF, ":/icons/svg-default/export-off.svg", ":/icons/svg-default/export-off-white.svg");
  initActionIcon(fileActionExportWRL, ":/icons/svg-default/export-wrl.svg", ":/icons/svg-default/export-wrl-white.svg");
  initActionIcon(fileActionExportPOV, ":/icons/svg-default/export-pov.svg", ":/icons/svg-default/export-pov-white.svg");
  initActionIcon(fileActionExportDXF, ":/icons/svg-default/export-dxf.svg", ":/icons/svg-default/export-dxf-white.svg");
  initActionIcon(fileActionExportSVG, ":/icons/svg-default/export-svg.svg", ":/icons/svg-default/export-svg-white.svg");
  initActionIcon(fileActionExportCSG, ":/icons/svg-default/export-csg.svg", ":/icons/svg-default/export-csg-white.svg");
  initActionIcon(fileActionExportPDF, ":/icons/svg-default/export-pdf.svg", ":/icons/svg-default/export-pdf-white.svg");
  initActionIcon(fileActionExportImage, ":/icons/svg-default/export-png.svg", ":/icons/svg-default/export-png-white.svg");
  initActionIcon(viewActionViewAll, ":/icons/svg-default/zoom-all.svg", ":/icons/svg-default/zoom-all-white.svg");
  initActionIcon(editActionUndo, ":/icons/svg-default/undo.svg", ":/icons/svg-default/undo-white.svg");
  initActionIcon(editActionRedo, ":/icons/svg-default/redo.svg", ":/icons/svg-default/redo-white.svg");
  initActionIcon(editActionUnindent, ":/icons/svg-default/unindent.svg", ":/icons/svg-default/unindent-white.svg");
  initActionIcon(editActionIndent, ":/icons/svg-default/indent.svg", ":/icons/svg-default/indent-white.svg");
  initActionIcon(viewActionResetView, ":/icons/svg-default/reset-view.svg", ":/icons/svg-default/reset-view-white.svg");
  initActionIcon(viewActionShowScaleProportional, ":/icons/svg-default/scalemarkers.svg", ":/icons/svg-default/scalemarkers-white.svg");

  InputDriverManager::instance()->registerActions(this->menuBar()->actions(), "", "");
  InputDriverManager::instance()->registerActions(this->animateWidget->actions(), "animation", "animate");
  instance->ButtonConfig->init();

  // fetch window states to be restored after restoreState() call
  bool hideConsole = settings.value("view/hideConsole").toBool();
  bool hideEditor = settings.value("view/hideEditor").toBool();
  bool hideCustomizer = settings.value("view/hideCustomizer").toBool();
  bool hideErrorLog = settings.value("view/hideErrorLog").toBool();
  bool hideAnimate = settings.value("view/hideAnimate").toBool();
  bool hideFontList = settings.value("view/hideFontList").toBool();
  bool hideViewportControl = settings.value("view/hideViewportControl").toBool();
  bool hideEditorToolbar = settings.value("view/hideEditorToolbar").toBool();
  bool hide3DViewToolbar = settings.value("view/hide3DViewToolbar").toBool();

  // make sure it looks nice..
  const auto windowState = settings.value("window/state", QByteArray()).toByteArray();
  restoreGeometry(settings.value("window/geometry", QByteArray()).toByteArray());
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
    tabifyDockWidget(fontListDock, animateDock);
    showConsole();
    hideCustomizer = true;
    hideViewportControl = true;
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
#endif // ifdef Q_OS_WIN
  }

  updateWindowSettings(hideConsole, hideEditor, hideCustomizer, hideErrorLog, hideEditorToolbar, hide3DViewToolbar, hideAnimate, hideFontList, hideViewportControl);

  connect(this->editorDock, SIGNAL(topLevelChanged(bool)), this, SLOT(editorTopLevelChanged(bool)));
  connect(this->consoleDock, SIGNAL(topLevelChanged(bool)), this, SLOT(consoleTopLevelChanged(bool)));
  connect(this->parameterDock, SIGNAL(topLevelChanged(bool)), this, SLOT(parameterTopLevelChanged(bool)));
  connect(this->errorLogDock, SIGNAL(topLevelChanged(bool)), this, SLOT(errorLogTopLevelChanged(bool)));
  connect(this->animateDock, SIGNAL(topLevelChanged(bool)), this, SLOT(animateTopLevelChanged(bool)));
  connect(this->fontListDock, SIGNAL(topLevelChanged(bool)), this, SLOT(fontListTopLevelChanged(bool)));
  connect(this->viewportControlDock, SIGNAL(topLevelChanged(bool)), this, SLOT(viewportControlTopLevelChanged(bool)));

  connect(this->activeEditor, SIGNAL(escapePressed()), this, SLOT(measureFinished()));
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

  for (int i = 1; i < filenames.size(); ++i)
    tabManager->createTab(filenames[i]);

  updateExportActions();

  this->selector = std::make_unique<MouseSelector>(this->qglview);
  activeEditor->setFocus();
}

void MainWindow::updateExportActions() {
  removeExportActions(editortoolbar, this->designAction3DPrint);
  addExportActions(this, editortoolbar, this->designAction3DPrint);

  //handle the hide/show of export action in view toolbar according to the visibility of editor dock
  removeExportActions(viewerToolBar, this->viewActionViewAll);
  if (!editorDock->isVisible()) {
    addExportActions(this, viewerToolBar, this->viewActionViewAll);
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

bool MainWindow::isLightTheme(){
  int defaultcolor = viewerToolBar->palette().window().color().lightness();
  return (defaultcolor > 165);
}

void MainWindow::initActionIcon(QAction *action, const char *darkResource, const char *lightResource)
{
  const char *resource = this->isLightTheme() ? darkResource : lightResource;
  action->setIcon(QIcon(resource));
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

    const QString toolTip("%1 &nbsp;<span style=\"color: gray; font-size: small; font-style: italic\">%2</span>");
    action->setToolTip(toolTip.arg(action->toolTip(), shortCut));
  }
}

/**
 * Update window settings that get overwritten by the restoreState()
 * Qt call. So the values are loaded before the call and restored here
 * regardless of the (potential outdated) serialized state.
 */
void MainWindow::updateWindowSettings(bool console, bool editor, bool customizer, bool errorLog, bool editorToolbar, bool viewToolbar, bool animate, bool fontList, bool viewportControl)
{
  windowActionHideEditor->setChecked(editor);
  hideEditor();
  windowActionHideConsole->setChecked(console);
  hideConsole();
  windowActionHideErrorLog->setChecked(errorLog);
  hideErrorLog();
  windowActionHideCustomizer->setChecked(customizer);
  hideParameters();
  windowActionHideAnimate->setChecked(animate);
  hideAnimate();
  windowActionHideFontList->setChecked(fontList);
  hideFontList();
  windowActionHideViewportControl->setChecked(viewportControl);
  hideViewportControl();

  viewActionHideEditorToolBar->setChecked(editorToolbar);
  hideEditorToolbar();
  viewActionHide3DViewToolBar->setChecked(viewToolbar);
  hide3DViewToolbar();
}

void MainWindow::onAxisChanged(InputEventAxisChanged *)
{

}

void MainWindow::onButtonChanged(InputEventButtonChanged *)
{

}

void MainWindow::onTranslateEvent(InputEventTranslate *event)
{
  double zoomFactor = 0.001 * qglview->cam.zoomValue();

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
  std::string actionName = event->action;
  if (actionName.find("::") == std::string::npos) {
    QAction *action = findAction(this->menuBar()->actions(), actionName);
    if (action) {
      action->trigger();
    } else if ("viewActionTogglePerspective" == actionName) {
      viewTogglePerspective();
    }
  } else {
    std::string target = actionName.substr(0, actionName.find("::"));
    if ("animate" == target) {
      this->animateWidget->onActionEvent(event);
    } else {
      std::cout << "unknown onActionEvent target: " << actionName << std::endl;
    }
  }
}

void MainWindow::onZoomEvent(InputEventZoom *event)
{
  qglview->zoom(event->zoom, event->relative);
}

void MainWindow::loadViewSettings(){
  QSettingsCached settings;

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

  updateUndockMode(Preferences::inst()->getValue("advanced/undockableWindows").toBool());
  updateReorderMode(Preferences::inst()->getValue("advanced/reorderWindows").toBool());
}

void MainWindow::loadDesignSettings()
{
  QSettingsCached settings;
  if (settings.value("design/autoReload", false).toBool()) {
    designActionAutoReload->setChecked(true);
  }
  auto polySetCacheSizeMB = Preferences::inst()->getValue("advanced/polysetCacheSizeMB").toUInt();
  GeometryCache::instance()->setMaxSizeMB(polySetCacheSizeMB);
  auto cgalCacheSizeMB = Preferences::inst()->getValue("advanced/cgalCacheSizeMB").toUInt();
  CGALCache::instance()->setMaxSizeMB(cgalCacheSizeMB);
  auto backend3D = Preferences::inst()->getValue("advanced/renderBackend3D").toString().toStdString();
  RenderSettings::inst()->backend3D = renderBackend3DFromString(backend3D);
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

    if (viewportControlDock->isFloating()) {
      viewportControlDock->setFloating(false);
    }
    viewportControlDock->setFeatures(viewportControlDock->features() & ~QDockWidget::DockWidgetFloatable);
  }
}

void MainWindow::updateReorderMode(bool reorderMode)
{
  MainWindow::reorderMode = reorderMode;
  editorDock->setTitleBarWidget(reorderMode ? nullptr : editorDockTitleWidget);
  consoleDock->setTitleBarWidget(reorderMode ? nullptr : consoleDockTitleWidget);
  parameterDock->setTitleBarWidget(reorderMode ? nullptr : parameterDockTitleWidget);
  errorLogDock->setTitleBarWidget(reorderMode ? nullptr : errorLogDockTitleWidget);
  animateDock->setTitleBarWidget(reorderMode ? nullptr : animateDockTitleWidget);
  fontListDock->setTitleBarWidget(reorderMode ? nullptr : fontListDockTitleWidget);
  viewportControlDock->setTitleBarWidget(reorderMode ? nullptr : viewportControlWidget);
}

MainWindow::~MainWindow()
{
  // If root_file is not null then it will be the same as parsed_file,
  // so no need to delete it.
  delete parsed_file;
  scadApp->windowManager.remove(this);
  if (scadApp->windowManager.getWindows().size() == 0) {
    // Quit application even in case some other windows like
    // Preferences are still open.
    this->quit();
  }
}

void MainWindow::showProgress()
{
  updateStatusBar(qobject_cast<ProgressWidget *>(sender()));
}

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
  QMetaObject::invokeMethod(this->progresswidget, "setValue", Qt::QueuedConnection, Q_ARG(int, (int)permille));
  return (progresswidget && progresswidget->wasCanceled());
}

void MainWindow::updateRecentFiles(const QString& FileSavedOrOpened)
{
  // Check that the canonical file path exists - only update recent files
  // if it does. Should prevent empty list items on initial open etc.
  QSettingsCached settings; // already set up properly via main.cpp
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

void MainWindow::setTabToolBarVisible(int count)
{
  tabCount = count;
  tabToolBar->setVisible((tabCount > 1) && editorDock->isVisible());
}

/*!
   compiles the design. Calls compileDone() if anything was compiled
 */
void MainWindow::compile(bool reload, bool forcedone)
{
  OpenSCAD::hardwarnings = Preferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  OpenSCAD::traceDepth = Preferences::inst()->getValue("advanced/traceDepth").toUInt();
  OpenSCAD::traceUsermoduleParameters = Preferences::inst()->getValue("advanced/enableTraceUsermoduleParameters").toBool();
  OpenSCAD::parameterCheck = Preferences::inst()->getValue("advanced/enableParameterCheck").toBool();
  OpenSCAD::rangeCheck = Preferences::inst()->getValue("advanced/enableParameterRangeCheck").toBool();

  try{
    bool shouldcompiletoplevel = false;
    bool didcompile = false;

    compileErrors = 0;
    compileWarnings = 0;

    this->renderStatistic.start();

    // Reload checks the timestamp of the toplevel file and refreshes if necessary,
    if (reload) {
      // Refresh files if it has changed on disk
      if (fileChangedOnDisk() && checkEditorModified()) {
        shouldcompiletoplevel = tabManager->refreshDocument(); // don't compile if we couldn't open the file
        if (shouldcompiletoplevel && Preferences::inst()->getValue("advanced/autoReloadRaise").toBool()) {
          // reloading the 'same' document brings the 'old' one to front.
          this->raise();
        }
      }
      // If the file hasn't changed, we might still need to compile it
      // if we haven't yet compiled the current text.
      else {
        auto current_doc = activeEditor->toPlainText();
        if (current_doc.size() && last_compiled_doc.size() == 0) {
          shouldcompiletoplevel = true;
        }
      }
    } else {
      shouldcompiletoplevel = true;
    }

    if (this->parsed_file) {
      auto mtime = this->parsed_file->includesChanged();
      if (mtime > this->includes_mtime) {
        this->includes_mtime = mtime;
        shouldcompiletoplevel = true;
      }
    }
    // Parsing and dependency handling must run to completion even with stop on errors to prevent auto
    // reload picking up where it left off, thwarting the stop, so we turn off exceptions in PRINT.
    no_exceptions_for_warnings();
    if (shouldcompiletoplevel) {
      initialize_rng();
      this->errorLogWidget->clearModel();
      if (Preferences::inst()->getValue("advanced/consoleAutoClear").toBool()) {
        this->console->actionClearConsole_triggered();
      }
      if (activeEditor->isContentModified()) saveBackup();
      parseTopLevelDocument();
      didcompile = true;
    }

    if (didcompile && parser_error_pos != last_parser_error_pos) {
      if (last_parser_error_pos >= 0) emit unhighlightLastError();
      if (parser_error_pos >= 0) emit highlightError(parser_error_pos);
      last_parser_error_pos = parser_error_pos;
    }

    if (this->root_file) {
      auto mtime = this->root_file->handleDependencies();
      if (mtime > this->deps_mtime) {
        this->deps_mtime = mtime;
        LOG("Used file cache size: %1$d files", SourceFileCache::instance()->size());
        didcompile = true;
      }
    }

    // Had any errors in the parse that would have caused exceptions via PRINT.
    if (would_have_thrown()) throw HardWarningException("");
    // If we're auto-reloading, listen for a cascade of changes by starting a timer
    // if something changed _and_ there are any external dependencies
    if (reload && didcompile && this->root_file) {
      if (this->root_file->hasIncludes() || this->root_file->usesLibraries()) {
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
  auto mtime = this->root_file->handleDependencies();
  auto stop = would_have_thrown();
  if (mtime > this->deps_mtime) this->deps_mtime = mtime;
  else if (!stop) {
    compile(true, true); // In case file itself or top-level includes changed during dependency updates
    return;
  }
  this->waitAfterReloadTimer->start();
}

void MainWindow::on_toolButtonCompileResultClose_clicked()
{
  frameCompileResult->hide();
}

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
      QFileInfo fileInfo(activeEditor->filepath);
      msg = QString(_("Error while compiling '%1'.")).arg(fileInfo.fileName());
    }
    toolButtonCompileResultIcon->setIcon(QIcon(QString::fromUtf8(":/icons/information-icons-error.png")));
  } else {
    const char *fmt = ngettext("Compilation generated %1 warning.", "Compilation generated %1 warnings.", compileWarnings);
    msg = QString(fmt).arg(compileWarnings);
    toolButtonCompileResultIcon->setIcon(QIcon(QString::fromUtf8(":/icons/information-icons-warning.png")));
  }
  QFontMetrics fm(labelCompileResultMessage->font());
  int sizeIcon = std::max(12, std::min(32, fm.height()));
  int sizeClose = std::max(10, std::min(32, fm.height()) - 4);
  toolButtonCompileResultIcon->setIconSize(QSize(sizeIcon, sizeIcon));
  toolButtonCompileResultClose->setIconSize(QSize(sizeClose, sizeClose));

  msg += _(R"( For details see the <a href="#errorlog">error log</a> and <a href="#console">console window</a>.)");
  labelCompileResultMessage->setText(msg);
  frameCompileResult->show();
}

void MainWindow::compileDone(bool didchange)
{
  OpenSCAD::hardwarnings = Preferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  try{
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
}

void MainWindow::instantiateRoot()
{
  // Go on and instantiate root_node, then call the continuation slot

  // Invalidate renderers before we kill the CSG tree
  this->qglview->setRenderer(nullptr);
#ifdef ENABLE_OPENCSG
  this->opencsgRenderer = nullptr;
#endif
  this->thrownTogetherRenderer = nullptr;

  // Remove previous CSG tree
  this->absolute_root_node.reset();

  this->csgRoot.reset();
  this->normalizedRoot.reset();
  this->root_products.reset();

  this->root_node.reset();
  this->tree.setRoot(nullptr);

  std::filesystem::path doc(activeEditor->filepath.toStdString());
  this->tree.setDocumentPath(doc.parent_path().string());

  if (this->root_file) {
    // Evaluate CSG tree
    LOG("Compiling design (CSG Tree generation)...");
    this->processEvents();

    AbstractNode::resetIndexCounter();

    EvaluationSession session{doc.parent_path().string()};
    ContextHandle<BuiltinContext> builtin_context{Context::create<BuiltinContext>(&session)};
    setRenderVariables(builtin_context);

    std::shared_ptr<const FileContext> file_context;
#ifdef ENABLE_PYTHON
    if (python_result_node != NULL && this->python_active) this->absolute_root_node = python_result_node;
    else
#endif
    this->absolute_root_node = this->root_file->instantiate(*builtin_context, &file_context);
    if (file_context) {
      this->qglview->cam.updateView(file_context, false);
      viewportControlWidget->cameraChanged();
    }

    if (this->absolute_root_node) {
      // Do we have an explicit root node (! modifier)?
      const Location *nextLocation = nullptr;
      if (!(this->root_node = find_root_tag(this->absolute_root_node, &nextLocation))) {
        this->root_node = this->absolute_root_node;
      }
      if (nextLocation) {
        LOG(message_group::NONE, *nextLocation, builtin_context->documentRoot(), "More than one Root Modifier (!)");
      }

      // FIXME: Consider giving away ownership of root_node to the Tree, or use reference counted pointers
      this->tree.setRoot(this->root_node);
    }
  }

  if (!this->root_node) {
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
  OpenSCAD::hardwarnings = Preferences::inst()->getValue("advanced/enableHardwarnings").toBool();
  try{
    assert(this->root_node);
    LOG("Compiling design (CSG Products generation)...");
    this->processEvents();

    // Main CSG evaluation
    this->progresswidget = new ProgressWidget(this);
    connect(this->progresswidget, SIGNAL(requestShow()), this, SLOT(showProgress()));

    GeometryEvaluator geomevaluator(this->tree);
#ifdef ENABLE_OPENCSG
    CSGTreeEvaluator csgrenderer(this->tree, &geomevaluator);
#endif

    if (!isClosing) progress_report_prep(this->root_node, report_func, this);
    else return;
    try {
#ifdef ENABLE_OPENCSG
      this->processEvents();
      this->csgRoot = csgrenderer.buildCSGTree(*root_node);
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

    size_t normalizelimit = 2ul * Preferences::inst()->getValue("advanced/openCSGLimit").toUInt();
    CSGTreeNormalizer normalizer(normalizelimit);

    if (this->csgRoot) {
      this->normalizedRoot = normalizer.normalize(this->csgRoot);
      if (this->normalizedRoot) {
        this->root_products.reset(new CSGProducts());
        this->root_products->import(this->normalizedRoot);
      } else {
        this->root_products.reset();
        LOG(message_group::Warning, "CSG normalization resulted in an empty tree");
        this->processEvents();
      }
    }

    const std::vector<std::shared_ptr<CSGNode>>& highlight_terms = csgrenderer.getHighlightNodes();
    if (highlight_terms.size() > 0) {
      LOG("Compiling highlights (%1$d CSG Trees)...", highlight_terms.size());
      this->processEvents();

      this->highlights_products.reset(new CSGProducts());
      for (const auto& highlight_term : highlight_terms) {
        auto nterm = normalizer.normalize(highlight_term);
        if (nterm) {
          this->highlights_products->import(nterm);
        }
      }
    } else {
      this->highlights_products.reset();
    }

    const auto& background_terms = csgrenderer.getBackgroundNodes();
    if (background_terms.size() > 0) {
      LOG("Compiling background (%1$d CSG Trees)...", background_terms.size());
      this->processEvents();

      this->background_products.reset(new CSGProducts());
      for (const auto& background_term : background_terms) {
        auto nterm = normalizer.normalize(background_term);
        if (nterm) {
          this->background_products->import(nterm);
        }
      }
    } else {
      this->background_products.reset();
    }

    if (this->root_products &&
        (this->root_products->size() >
         Preferences::inst()->getValue("advanced/openCSGLimit").toUInt())) {
      LOG(message_group::UI_Warning, "Normalized tree has %1$d elements!", this->root_products->size());
      LOG(message_group::UI_Warning, "OpenCSG rendering has been disabled.");
    }
#ifdef ENABLE_OPENCSG
    else {
      LOG("Normalized tree has %1$d elements!",
          (this->root_products ? this->root_products->size() : 0));
#ifdef USE_LEGACY_RENDERERS
      this->opencsgRenderer = std::make_shared<LegacyOpenCSGRenderer>(this->root_products,
                                                                      this->highlights_products,
                                                                      this->background_products);
#else
      this->opencsgRenderer = std::make_shared<OpenCSGRenderer>(this->root_products,
                                                                this->highlights_products,
                                                                this->background_products);
#endif
    }
#endif // ifdef ENABLE_OPENCSG
#ifdef USE_LEGACY_RENDERERS
    this->thrownTogetherRenderer = std::make_shared<LegacyThrownTogetherRenderer>(this->root_products,
                                                                                  this->highlights_products,
                                                                                  this->background_products);
#else
    this->thrownTogetherRenderer = std::make_shared<ThrownTogetherRenderer>(this->root_products,
                                                                            this->highlights_products,
                                                                            this->background_products);
#endif
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

void MainWindow::actionNewWindow()
{
  new MainWindow(QStringList());
}

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
  QSettingsCached settings; // already set up properly via main.cpp
  QStringList files;
  settings.setValue("recentFileList", files);

  updateRecentFileActions();
}

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
    auto examples = UIUtils::exampleFiles(cat);
    auto menu = this->menuExamples->addMenu(gettext(cat.toStdString().c_str()));

    for (const auto& ex : examples) {
      auto openAct = new QAction(ex.fileName().replace("&", "&&"), this);
      connect(openAct, SIGNAL(triggered()), this, SLOT(actionOpenExample()));
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
    this->tempFile = new QTemporaryFile(backupPath.append(basename + "-backup-XXXXXXXX.scad"));
  }

  if ((!this->tempFile->isOpen()) && (!this->tempFile->open())) {
    LOG(message_group::UI_Warning, "Failed to create backup file");
    return;
  }
  return writeBackup(this->tempFile);
}

void MainWindow::actionSave()
{
  tabManager->save(activeEditor);
}

void MainWindow::actionSaveAs()
{
  tabManager->saveAs(activeEditor);
}

void MainWindow::actionRevokeTrustedFiles()
{
  QSettingsCached settings;
#ifdef ENABLE_PYTHON
  python_trusted = false;
  this->trusted_edit_document_name = "";
#endif
  settings.remove("python_hash");
  QMessageBox::information(this, _("Trusted Files"), "All trusted python files revoked", QMessageBox::Ok);

}

void MainWindow::actionSaveACopy()
{
  tabManager->saveACopy(activeEditor);
}

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
    fileChangedOnDisk(); // force cached autoReloadId to update
    (void)tabManager->refreshDocument(); // ignore errors opening the file
  }
}

void MainWindow::copyViewportTranslation()
{
  const auto vpt = qglview->cam.getVpt();
  const QString txt = QString("[ %1, %2, %3 ]")
    .arg(vpt.x(), 0, 'f', 2)
    .arg(vpt.y(), 0, 'f', 2)
    .arg(vpt.z(), 0, 'f', 2);
  QApplication::clipboard()->setText(txt);
}

void MainWindow::copyViewportRotation()
{
  const auto vpr = qglview->cam.getVpr();
  const QString txt = QString("[ %1, %2, %3 ]")
    .arg(vpr.x(), 0, 'f', 2)
    .arg(vpr.y(), 0, 'f', 2)
    .arg(vpr.z(), 0, 'f', 2);
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
  this->findInputField->setFindCount(activeEditor->updateFindIndicators(this->findInputField->text(), false));
  this->processEvents();
}

void MainWindow::showFind()
{
  this->findInputField->setFindCount(activeEditor->updateFindIndicators(this->findInputField->text()));
  this->processEvents();
  findTypeComboBox->setCurrentIndex(0);
  replaceInputField->hide();
  replaceButton->hide();
  replaceAllButton->hide();
  //replaceLabel->setVisible(false);
  find_panel->show();
  activeEditor->findState = TabManager::FIND_VISIBLE;
  editActionFindNext->setEnabled(true);
  editActionFindPrevious->setEnabled(true);
  if (!activeEditor->selectedText().isEmpty()) {
    findInputField->setText(activeEditor->selectedText());
  }
  findInputField->setFocus();
  findInputField->selectAll();
}

void MainWindow::findString(const QString& textToFind)
{
  this->findInputField->setFindCount(activeEditor->updateFindIndicators(textToFind));
  this->processEvents();
  activeEditor->find(textToFind);
}

void MainWindow::showFindAndReplace()
{
  this->findInputField->setFindCount(activeEditor->updateFindIndicators(this->findInputField->text()));
  this->processEvents();
  findTypeComboBox->setCurrentIndex(1);
  replaceInputField->show();
  replaceButton->show();
  replaceAllButton->show();
  //replaceLabel->setVisible(true);
  find_panel->show();
  activeEditor->findState = TabManager::FIND_REPLACE_VISIBLE;
  editActionFindNext->setEnabled(true);
  editActionFindPrevious->setEnabled(true);
  if (!activeEditor->selectedText().isEmpty()) {
    findInputField->setText(activeEditor->selectedText());
  }
  findInputField->setFocus();
  findInputField->selectAll();
}

void MainWindow::selectFindType(int type)
{
  if (type == 0) showFind();
  if (type == 1) showFindAndReplace();
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

void MainWindow::findNext()
{
  activeEditor->find(this->findInputField->text(), true);
}

void MainWindow::findPrev()
{
  activeEditor->find(this->findInputField->text(), true, true);
}

void MainWindow::useSelectionForFind()
{
  findInputField->setText(activeEditor->selectedText());
}

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

bool MainWindow::event(QEvent *event) {
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
  RenderVariables r = {
    .preview = this->is_preview,
    .time = this->animateWidget->getAnim_tval(),
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
    bool valid = (stat(activeEditor->filepath.toLocal8Bit(), &st) == 0);
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
bool MainWindow::trust_python_file(const std::string& file,  const std::string& content) {
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

  if (content.size() <= 1) { // 1st character already typed
    this->trusted_edit_document_name = file;
    return true;
  }

  if (settings.contains(setting_key)) {
    QString str = settings.value(setting_key).toString();
    QByteArray ba = str.toLocal8Bit();
    ref_hash = std::string(ba.data());
  }

  if (act_hash == ref_hash) {
    this->trusted_edit_document_name = file;
    return true;
  }

  auto ret = QMessageBox::warning(this, "Application",
                                  _("Python files can potentially contain harmful stuff.\n"
                                    "Do you trust this file ?\n"), QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::No);
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
#endif // ifdef ENABLE_PYTHON

void MainWindow::parseTopLevelDocument()
{
  resetSuppressedMessages();

  this->last_compiled_doc = activeEditor->toPlainText();

  auto fulltext =
    std::string(this->last_compiled_doc.toUtf8().constData()) +
    "\n\x03\n" + commandline_commands;

  auto fnameba = activeEditor->filepath.toLocal8Bit();
  const char *fname = activeEditor->filepath.isEmpty() ? "" : fnameba;
  delete this->parsed_file;
#ifdef ENABLE_PYTHON
  this->python_active = false;
  if (fname != NULL) {
    if (boost::algorithm::ends_with(fname, ".py")) {
      std::string content = std::string(this->last_compiled_doc.toUtf8().constData());
      if (
        Feature::ExperimentalPythonEngine.is_enabled()
        && trust_python_file(std::string(fname), content)) this->python_active = true;
      else LOG(message_group::Warning, Location::NONE, "", "Python is not enabled");
    }
  }

  if (this->python_active) {
    auto fulltext_py =
      std::string(this->last_compiled_doc.toUtf8().constData());

    auto error = evaluatePython(fulltext_py, this->animateWidget->getAnim_tval());
    if (error.size() > 0) LOG(message_group::Error, Location::NONE, "", error.c_str());
    fulltext = "\n";
  }
#endif // ifdef ENABLE_PYTHON
  this->parsed_file = nullptr; // because the parse() call can throw and we don't want a stale pointer!
  this->root_file = nullptr;  // ditto
  this->root_file = parse(this->parsed_file, fulltext, fname, fname, false) ? this->parsed_file : nullptr;

  this->activeEditor->resetHighlighting();
  if (this->root_file != nullptr) {
    //add parameters as annotation in AST
    CommentParser::collectParameters(fulltext, this->root_file);
    this->activeEditor->parameterWidget->setParameters(this->root_file, fulltext);
    this->activeEditor->parameterWidget->applyParameters(this->root_file);
    this->activeEditor->parameterWidget->setEnabled(true);
    this->activeEditor->setIndicator(this->root_file->indicatorData);
  } else {
    this->activeEditor->parameterWidget->setEnabled(false);
  }
}

void MainWindow::changeParameterWidget()
{
  windowActionHideCustomizer->setVisible(true);
}

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
  this->is_preview = true;
  compile(true);
}

void MainWindow::csgReloadRender()
{
  if (this->root_node) compileCSG();

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
  this->is_preview = preview;
}

void MainWindow::actionRenderPreview()
{
  static bool preview_requested;

  preview_requested = true;
  if (GuiLocker::isLocked()) return;
  GuiLocker::lock();
  preview_requested = false;

  this->designActionMeasureDist->setEnabled(false);
  this->designActionMeasureAngle->setEnabled(false);

  prepareCompile("csgRender", windowActionHideAnimate->isChecked(), true);
  compile(false, false);
  if (preview_requested) {
    // if the action was called when the gui was locked, we must request it one more time
    // however, it's not possible to call it directly NOR make the loop
    // it must be called from the mainloop
    QTimer::singleShot(0, this, SLOT(actionRenderPreview()));
  }
}

void MainWindow::csgRender()
{
  if (this->root_node) compileCSG();

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

  if (animateWidget->dumpPictures() ) {
    int steps = animateWidget->nextFrame();
    QImage img = this->qglview->grabFrame();
    QString filename = QString("frame%1.png").arg(steps, 5, 10, QChar('0'));
    img.save(filename, "PNG");
  }

  compileEnded();
}

std::unique_ptr<ExternalToolInterface> createExternalToolService(
  print_service_t serviceType, const QString& serviceName, FileFormat fileFormat)
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
    case print_service_t::OCTOPRINT:
      return createOctoPrintService(fileFormat);
    break;
    case print_service_t::LOCALSLICER:
      return createLocalProgramService(fileFormat);
    break;
  }
  return {};
}

void MainWindow::sendToExternalTool(ExternalToolInterface &externalToolService)
{
  const QFileInfo activeFile(activeEditor->filepath);
  QString activeFileName = activeFile.fileName();
  if (activeFileName.isEmpty()) activeFileName = "Untitled.scad";
  // TODO: Replace suffix to match exported file format?
  
  activeFileName = activeFileName + QString::fromStdString("." + fileformat::toSuffix(externalToolService.fileFormat()));

  bool export_status = externalToolService.exportTemporaryFile(this->root_geom, activeFileName, &qglview->cam);
  
  this->progresswidget = new ProgressWidget(this);
  connect(this->progresswidget, SIGNAL(requestShow()), this, SLOT(showProgress()));

  bool process_status = externalToolService.process(activeFileName.toStdString(), [this](double permille) {
    return network_progress_func(permille);
  });
  updateStatusBar(nullptr);

  const auto url = externalToolService.getURL();
  if (!url.empty()) {
    QDesktopServices::openUrl(QUrl{QString::fromStdString(url)});
  }
}

void MainWindow::action3DPrint()
{
#ifdef ENABLE_3D_PRINTING
  if (GuiLocker::isLocked()) return;
  GuiLocker lock;

  setCurrentOutput();

  //Make sure we can export:
  const unsigned int dim = 3;
  if (!canExport(dim)) return;

  PrintInitDialog printInitDialog;
  const auto status = printInitDialog.exec();

  if (status == QDialog::Accepted) {
    const print_service_t serviceType = printInitDialog.getServiceType();
    const QString serviceName = printInitDialog.getServiceName();
    const FileFormat fileFormat = printInitDialog.getFileFormat();

    LOG("Selected File format: %1$s", fileformat::info(fileFormat).description);


    Preferences::Preferences::inst()->updateGUI();
    const auto externalToolService = createExternalToolService(serviceType, serviceName, fileFormat);
    if (!externalToolService) {
      LOG("Error: Unable to create service: %1$d %2$s %3$d", static_cast<int>(serviceType), serviceName.toStdString(), static_cast<int>(fileFormat));
      return;
    }
    sendToExternalTool(*externalToolService);
  }
#endif // ifdef ENABLE_3D_PRINTING
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
  if (!this->root_file || !this->root_node) {
    compileEnded();
    return;
  }

  this->qglview->setRenderer(nullptr);
  this->cgalRenderer = nullptr;
  this->root_geom.reset();

  LOG("Rendering Polygon Mesh using %1$s...",
      renderBackend3DToString(RenderSettings::inst()->backend3D).c_str());

  this->progresswidget = new ProgressWidget(this);
  connect(this->progresswidget, SIGNAL(requestShow()), this, SLOT(showProgress()));

  if (!isClosing) progress_report_prep(this->root_node, report_func, this);
  else return;

  this->cgalworker->start(this->tree);
}

void MainWindow::actionRenderDone(const std::shared_ptr<const Geometry>& root_geom)
{
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

    this->root_geom = root_geom;
#ifdef USE_LEGACY_RENDERERS
    this->cgalRenderer = std::make_shared<LegacyCGALRenderer>(root_geom);
#else
    this->cgalRenderer = std::make_shared<CGALRenderer>(root_geom);
#endif
    // Go to CGAL view mode
    if (viewActionWireframe->isChecked()) viewModeWireframe();
    else viewModeSurface();
    this->designActionMeasureDist->setEnabled(true);
    this->designActionMeasureAngle->setEnabled(true);
  } else {
    this->designActionMeasureDist->setEnabled(false);
    this->designActionMeasureAngle->setEnabled(false);
    LOG(message_group::UI_Warning, "No top level geometry to render");
  }

  updateStatusBar(nullptr);

  const bool renderSoundEnabled = Preferences::inst()->getValue("advanced/enableSoundNotification").toBool();
  const uint soundThreshold = Preferences::inst()->getValue("advanced/timeThresholdOnRenderCompleteSound").toUInt();
  if (renderSoundEnabled && soundThreshold <= renderStatistic.ms().count() / 1000) {
    renderCompleteSoundEffect->play();
  }

  renderedEditor = activeEditor;
  activeEditor->contentsRendered = true;
  compileEnded();
}

void MainWindow::actionMeasureDistance()
{
  meas.startMeasureDist();
}

void MainWindow::actionMeasureAngle()
{
  meas.startMeasureAngle();
}

void MainWindow::leftClick(QPoint mouse)
{
  QString str = meas.statemachine(mouse);
  if (str.size() > 0) {
    this->qglview->measure_state = MEASURE_IDLE;
    QMenu resultmenu(this);
    auto action = resultmenu.addAction(str);
    connect(action, SIGNAL(triggered()), this, SLOT(measureFinished()));
    resultmenu.exec(qglview->mapToGlobal(mouse));
  }
}

/**
 * Call the mouseselection to determine the id of the clicked-on object.
 * Use the generated ID and try to find it within the list of products
 * And finally move the cursor to the beginning of the selected object in the editor
 */
void MainWindow::rightClick(QPoint mouse)
{
  // selecting without a renderer?!
  if (!this->qglview->renderer) {
    return;
  }

  // selecting without select object?!
  if (!this->selector) {
    return;
  }

  // Nothing to select
  if (!this->root_products) {
    return;
  }

  this->qglview->renderer->prepare(true, false, &this->selector->shaderinfo);

  // Update the selector with the right image size
  this->selector->reset(this->qglview);

  // Select the object at mouse coordinates
  int index = this->selector->select(this->qglview->getRenderer(), mouse.x(), mouse.y());
  std::deque<std::shared_ptr<const AbstractNode>> path;
  std::shared_ptr<const AbstractNode> result = this->root_node->getNodeByID(index, path);

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

      // Remove the "module" prefix if any as it induce confusion between the module declaration and instanciation
      int first_position = (step->verbose_name().find("module") == std::string::npos)? 0 : 7;
      std::string name = step->verbose_name().substr(first_position);

      // It happens that the verbose_name is empty (eg: in for loops), when this happens instead of letting
      // empty entry in the menu we prefer using the name in the modinstanciation.
      if (step->verbose_name().empty()) name = step->modinst->name();

      // Check if the path is contained in a library (using parsersettings.h)
      fs::path libpath = get_library_for_path(location.filePath());
      if (!libpath.empty()) {
        // Display the library (without making the window too wide!)
        ss << name << " (library "
           << location.fileName().substr(libpath.string().length() + 1) << ":"
           << location.firstLine() << ")";
      } else if (activeEditor->filepath.toStdString() == location.fileName()) {
        // removes the "module" prefix if any as it makes it not clear if it is module declaration or call.
        ss << name << " (" << location.filePath().filename().string() << ":"
           << location.firstLine() << ")";
      } else {
        auto relative_filename = fs_uncomplete(location.filePath(), fs::path(activeEditor->filepath.toStdString()).parent_path())
          .generic_string();
        // Set the displayed name relative to the active editor window
        ss << name << " (" << relative_filename << ":" << location.firstLine() << ")";
      }

      // Prepare the action to be sent
      auto action = tracemenu.addAction(QString::fromStdString(ss.str()));
      if (editorDock->isVisible()) {
        action->setProperty("id", step->idx);
        connect(action, SIGNAL(hovered()), this, SLOT(onHoveredObjectInSelectionMenu()));
      }
    }

    tracemenu.exec(this->qglview->mapToGlobal(mouse));
  } else {
    clearAllSelectionIndicators();
  }
}

void MainWindow::measureFinished()
{
  this->qglview->selected_obj.clear();
  this->qglview->shown_obj.clear();
  this->qglview->update();
  this->qglview->measure_state = MEASURE_IDLE;
}

void MainWindow::clearAllSelectionIndicators()
{
  this->activeEditor->clearAllSelectionIndicators();
}

void findNodesWithSameMod(std::shared_ptr<const AbstractNode> tree,
                          std::shared_ptr<const AbstractNode> node_mod,
                          std::vector<std::shared_ptr<const AbstractNode>>& nodes){
  if (node_mod->modinst == tree->modinst) {
    nodes.push_back(tree);
  }
  for (const auto& step : tree->children) {
    findNodesWithSameMod(step, node_mod, nodes);
  }
}

void getCodeLocation(const AbstractNode *self, int currentLevel,  int includeLevel, int *firstLine, int *firstColumn, int *lastLine, int *lastColumn, int nestedModuleDepth)
{
  auto location = self->modinst->location();
  if (currentLevel >= includeLevel && nestedModuleDepth == 0) {
    if (*firstLine < 0 || *firstLine > location.firstLine()) {
      *firstLine = location.firstLine();
      *firstColumn = location.firstColumn();
    } else if (*firstLine == location.firstLine() && *firstColumn > location.firstColumn())   {
      *firstColumn = location.firstColumn();
    }

    if (*lastLine < 0 || *lastLine < location.lastLine()) {
      *lastLine = location.lastLine();
      *lastColumn = location.lastColumn();
    } else {
      if (*firstLine < 0 || *firstLine > location.firstLine()) {
        *firstLine = location.firstLine();
        *firstColumn = location.firstColumn();
      } else if (*firstLine == location.firstLine() && *firstColumn > location.firstColumn())   {
        *firstColumn = location.firstColumn();
      }
      if (*lastLine < 0 || *lastLine < location.lastLine()) {
        *lastLine = location.lastLine();
        *lastColumn = location.lastColumn();
      } else if (*lastLine == location.lastLine() && *lastColumn < location.lastColumn())   {
        *lastColumn = location.lastColumn();
      }
    }
  }

  if (self->verbose_name().rfind("module", 0) == 0) {
    nestedModuleDepth++;
  }
  if (self->modinst->name() == "children") {
    nestedModuleDepth--;
  }

  if (nestedModuleDepth >= 0) {
    for (const auto& node : self->children) {
      getCodeLocation(node.get(), currentLevel + 1, includeLevel, firstLine,  firstColumn, lastLine, lastColumn, nestedModuleDepth);
    }
  }
}

void MainWindow::setSelectionIndicatorStatus(int nodeIndex, EditorSelectionIndicatorStatus status)
{
  std::deque<std::shared_ptr<const AbstractNode>> stack;
  this->root_node->getNodeByID(nodeIndex, stack);

  int level = 1;

  // first we flags all the nodes in the stack of the provided index
  // ends at size - 1 because we are not doing anything for the root node.
  // starts at 1 because we will process this one after later
  for (int i = 1; i < stack.size() - 1; i++) {
    const auto& node = stack[i];

    auto& location = node->modinst->location();
    if (location.filePath().compare(activeEditor->filepath.toStdString()) != 0) {
      std::cout << "--->>> Line of code in a different file -- PATH -- " << location.fileName() << std::endl;
      node->modinst->print(std::cout, "");
      level++;
      continue;
    }

    if (node->verbose_name().rfind("module", 0) == 0 || node->modinst->name() == "children") {
      this->activeEditor->setSelectionIndicatorStatus(
        status, level,
        location.firstLine() - 1, location.firstColumn() - 1, location.lastLine() - 1, location.lastColumn() - 1);
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
  getCodeLocation(node.get(), 0, 0, &line, &column, &lastLine, &lastColumn, 0);

  this->activeEditor->setSelectionIndicatorStatus(status, 0, line - 1, column - 1, lastLine - 1, lastColumn - 1);
}

void MainWindow::setSelection(int index)
{
  if (currently_selected_object == index) return;

  std::deque<std::shared_ptr<const AbstractNode>> path;
  std::shared_ptr<const AbstractNode> selected_node = root_node->getNodeByID(index, path);

  if (!selected_node) return;

  currently_selected_object = index;

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
  clearAllSelectionIndicators();

  std::vector<std::shared_ptr<const AbstractNode>> nodesSameModule{};
  findNodesWithSameMod(root_node, selected_node, nodesSameModule);

  // highlight in the text editor all the text fragment of the hierarchy of object with same mode.
  for (const auto& element : nodesSameModule) {
    if (element->index() != currently_selected_object) {
      setSelectionIndicatorStatus(element->index(), EditorSelectionIndicatorStatus::IMPACTED);
    }
  }

  // highlight in the text editor only the fragment correponding to the selected stack.
  // this step must be done after all the impacted element have been marked.
  setSelectionIndicatorStatus(currently_selected_object, EditorSelectionIndicatorStatus::SELECTED);

  activeEditor->setCursorPosition(line - 1, column - 1);
}

/**
 * Expects the sender to have properties "id" defined
 */
void MainWindow::onHoveredObjectInSelectionMenu()
{
  auto *action = qobject_cast<QAction *>(sender());
  if (!action || !action->property("id").isValid()) {
    return;
  }

  setSelection(action->property("id").toInt());
}

void MainWindow::setLastFocus(QWidget *widget) {
  this->lastFocus = widget;
}

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
      versionLabel = new QLabel("OpenSCAD " + QString::fromStdString(openscad_displayversionnumber));
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

void MainWindow::exceptionCleanup(){
  LOG("Execution aborted");
  LOG(" ");
  GuiLocker::unlock();
  if (designActionAutoReload->isChecked()) autoReloadTimer->start();
}

void MainWindow::UnknownExceptionCleanup(std::string msg){
  setCurrentOutput(); // we need to show this error
  if (msg.size() == 0) {
    LOG(message_group::Error, "Compilation aborted by unknown exception");
  } else {
    LOG(message_group::Error, "Compilation aborted by exception: %1$s", msg);
  }
  LOG(" ");
  GuiLocker::unlock();
  if (designActionAutoReload->isChecked()) autoReloadTimer->start();
}

void MainWindow::actionDisplayAST()
{
  setCurrentOutput();
  auto e = new QTextEdit(this);
  e->setAttribute(Qt::WA_DeleteOnClose);
  e->setWindowFlags(Qt::Window);
  e->setTabStopDistance(tabStopWidth);
  e->setWindowTitle("AST Dump");
  e->setReadOnly(true);
  if (root_file) {
    e->setPlainText(QString::fromStdString(root_file->dump("")));
  } else {
    e->setPlainText("No AST to dump. Please try compiling first...");
  }
  e->resize(600, 400);
  e->show();
  clearCurrentOutput();
}

void MainWindow::actionDisplayCSGTree()
{
  setCurrentOutput();
  auto e = new QTextEdit(this);
  e->setAttribute(Qt::WA_DeleteOnClose);
  e->setWindowFlags(Qt::Window);
  e->setTabStopDistance(tabStopWidth);
  e->setWindowTitle("CSG Tree Dump");
  e->setReadOnly(true);
  if (this->root_node) {
    e->setPlainText(QString::fromStdString(this->tree.getString(*this->root_node, "  ")));
  } else {
    e->setPlainText("No CSG to dump. Please try compiling first...");
  }
  e->resize(600, 400);
  e->show();
  clearCurrentOutput();
}

void MainWindow::actionDisplayCSGProducts()
{
  std::string NA("N/A");
  setCurrentOutput();
  auto e = new QTextEdit(this);
  e->setAttribute(Qt::WA_DeleteOnClose);
  e->setWindowFlags(Qt::Window);
  e->setTabStopDistance(tabStopWidth);
  e->setWindowTitle("CSG Products Dump");
  e->setReadOnly(true);
  e->setPlainText(QString("\nCSG before normalization:\n%1\n\n\nCSG after normalization:\n%2\n\n\nCSG rendering chain:\n%3\n\n\nHighlights CSG rendering chain:\n%4\n\n\nBackground CSG rendering chain:\n%5\n")

                  .arg(QString::fromStdString(this->csgRoot ? this->csgRoot->dump() : NA),
                       QString::fromStdString(this->normalizedRoot ? this->normalizedRoot->dump() : NA),
                       QString::fromStdString(this->root_products ? this->root_products->dump() : NA),
                       QString::fromStdString(this->highlights_products ? this->highlights_products->dump() : NA),
                       QString::fromStdString(this->background_products ? this->background_products->dump() : NA)));

  e->resize(600, 400);
  e->show();
  clearCurrentOutput();
}

void MainWindow::actionCheckValidity()
{
  if (GuiLocker::isLocked()) return;
  GuiLocker lock;
  setCurrentOutput();

  if (!this->root_geom) {
    LOG("Nothing to validate! Try building first (press F6).");
    clearCurrentOutput();
    return;
  }

  if (this->root_geom->getDimension() != 3) {
    LOG("Current top level object is not a 3D object.");
    clearCurrentOutput();
    return;
  }

  bool valid = true;
#ifdef ENABLE_CGAL 
 if (auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(this->root_geom)) {
    valid = N->p3 ? const_cast<CGAL_Nef_polyhedron3&>(*N->p3).is_valid() : false;
  } else if (auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(this->root_geom)) {
    valid = hybrid->isValid();
  } else
#endif
#ifdef ENABLE_MANIFOLD
  if (auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(this->root_geom)) {
    valid = mani->isValid();
  }
#endif
  LOG("Valid:      %1$6s", (valid ? "yes" : "no"));
  clearCurrentOutput();
}

//Returns if we can export (true) or not(false) (bool)
//Separated into it's own function for re-use.
bool MainWindow::canExport(unsigned int dim)
{
  if (!this->root_geom) {
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

  if (this->root_geom->getDimension() != dim) {
    LOG(message_group::UI_Error, "Current top level object is not a %1$dD object.", dim);
    clearCurrentOutput();
    return false;
  }

  if (this->root_geom->isEmpty()) {
    LOG(message_group::UI_Error, "Current top level object is empty.");
    clearCurrentOutput();
    return false;
  }

#ifdef ENABLE_CGAL
  auto N = dynamic_cast<const CGAL_Nef_polyhedron *>(this->root_geom.get());
  if (N && !N->p3->is_simple()) {
    LOG(message_group::UI_Warning, "Object may not be a valid 2-manifold and may need repair! See https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export");
  }
#endif
#ifdef ENABLE_MANIFOLD
  auto manifold = dynamic_cast<const ManifoldGeometry *>(this->root_geom.get());
  if (manifold && !manifold->isValid() ) {
    LOG(message_group::UI_Warning, "Object may not be a valid manifold and may need repair! "
      "Error message: %1$s. See https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/STL_Import_and_Export",
      ManifoldUtils::statusToString(manifold->getManifold().Status()));
  }
#endif

  return true;
}

void MainWindow::actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim) {
  ExportPdfOptions *empty = nullptr;
  actionExport(format, type_name, suffix, dim, empty);
}

void MainWindow::actionExport(FileFormat format, const char *type_name, const char *suffix, unsigned int dim, ExportPdfOptions *options)
{
  //Setting filename skips the file selection dialog and uses the path provided instead.
  if (GuiLocker::isLocked()) return;
  GuiLocker lock;

  setCurrentOutput();

  //Return if something is wrong and we can't export.
  if (!canExport(dim)) return;

  auto title = QString(_("Export %1 File")).arg(type_name);
  auto filter = QString(_("%1 Files (*%2)")).arg(type_name, suffix);
  auto exportFilename = QFileDialog::getSaveFileName(this, title, exportPath(suffix), filter);
  if (exportFilename.isEmpty()) {
    clearCurrentOutput();
    return;
  }
  this->export_paths[suffix] = exportFilename;

  ExportInfo exportInfo = {.format = format, .sourceFilePath = activeEditor->filepath.toStdString(), .camera = &qglview->cam};
  // Add options
  exportInfo.options = options;

  bool exportResult = exportFileByName(this->root_geom, exportFilename.toStdString(), exportInfo);

  if (exportResult) fileExportedMessage(type_name, exportFilename);
  clearCurrentOutput();
}

void MainWindow::actionExportFileFormat(int fmt)
{
  const FileFormat format = static_cast<FileFormat>(fmt);
  const FileFormatInfo &info = fileformat::info(format);
  const std::string suffix = "." + info.suffix;
  switch (format) {
    case FileFormat::PDF:
{

  ExportPdfOptions exportPdfOptions;
  QSettingsCached settings;

// Prepopulated with default values in export.h
  auto exportPdfDialog = new ExportPdfDialog();

// Get current settings or defaults
//  modify the two enums (next two rows) to explicitly use default by lookup to string (see the later set methods).
  exportPdfDialog->setPaperSize(sizeString2Enum(settings.value("exportPdfOpts/paperSize",
                                                               QString::fromStdString(paperSizeStrings[static_cast<int>(exportPdfOptions.paperSize)])).toString())); // enum map
  exportPdfDialog->setOrientation(orientationsString2Enum(settings.value("exportPdfOpts/orientation",
                                                                         QString::fromStdString(paperOrientationsStrings[static_cast<int>(exportPdfOptions.Orientation)])).toString())); // enum map
  exportPdfDialog->setShowDesignFilename(settings.value("exportPdfOpts/showDsgnFN", exportPdfOptions.showDesignFilename).toBool());
  exportPdfDialog->setShowScale(settings.value("exportPdfOpts/showScale", exportPdfOptions.showScale).toBool());
  exportPdfDialog->setShowScaleMsg(settings.value("exportPdfOpts/showScaleMsg", exportPdfOptions.showScaleMsg).toBool());
  exportPdfDialog->setShowGrid(settings.value("exportPdfOpts/showGrid", exportPdfOptions.showGrid).toBool());
  exportPdfDialog->setGridSize(settings.value("exportPdfOpts/gridSize", exportPdfOptions.gridSize).toDouble());


  if (exportPdfDialog->exec() == QDialog::Rejected) {
    return;
  }

  exportPdfOptions.paperSize = exportPdfDialog->getPaperSize();
  exportPdfOptions.Orientation = exportPdfDialog->getOrientation();
  exportPdfOptions.showDesignFilename = exportPdfDialog->getShowDesignFilename();
  exportPdfOptions.showScale = exportPdfDialog->getShowScale();
  exportPdfOptions.showScaleMsg = exportPdfDialog->getShowScaleMsg();
  exportPdfOptions.showGrid = exportPdfDialog->getShowGrid();
  exportPdfOptions.gridSize = exportPdfDialog->getGridSize();

  settings.setValue("exportPdfOpts/paperSize", QString::fromStdString(paperSizeStrings[static_cast<int>(exportPdfDialog->getPaperSize())]));
  settings.setValue("exportPdfOpts/orientation", QString::fromStdString(paperOrientationsStrings[static_cast<int>(exportPdfDialog->getOrientation())]));
  settings.setValue("exportPdfOpts/showDsgnFN", exportPdfDialog->getShowDesignFilename());
  settings.setValue("exportPdfOpts/showScale", exportPdfDialog->getShowScale());
  settings.setValue("exportPdfOpts/showScaleMsg", exportPdfDialog->getShowScaleMsg());
  settings.setValue("exportPdfOpts/showGrid", exportPdfDialog->getShowGrid());
  settings.setValue("exportPdfOpts/gridSize", exportPdfDialog->getGridSize());

  actionExport(FileFormat::PDF, "PDF", ".pdf", 2, &exportPdfOptions);

}
      break;
    case FileFormat::CSG:
{
  setCurrentOutput();

  if (!this->root_node) {
    LOG(message_group::Error, "Nothing to export. Please try compiling first.");
    clearCurrentOutput();
    return;
  }
  const auto suffix = ".csg";
  auto csg_filename = QFileDialog::getSaveFileName(this,
                                                   _("Export CSG File"), exportPath(suffix), _("CSG Files (*.csg)"));

  if (csg_filename.isEmpty()) {
    clearCurrentOutput();
    return;
  }

  std::ofstream fstream(csg_filename.toLocal8Bit());
  if (!fstream.is_open()) {
    LOG("Can't open file \"%1$s\" for export", csg_filename.toLocal8Bit().constData());
  } else {
    fstream << this->tree.getString(*this->root_node, "\t") << "\n";
    fstream.close();
    fileExportedMessage("CSG", csg_filename);
    this->export_paths[suffix] = csg_filename;
  }

  clearCurrentOutput();
}      break;
    case FileFormat::PNG:
{
  // Grab first to make sure dialog box isn't part of the grabbed image
  qglview->grabFrame();
  const auto suffix = ".png";
  auto img_filename = QFileDialog::getSaveFileName(this,
                                                   _("Export Image"), exportPath(suffix), _("PNG Files (*.png)"));
  if (!img_filename.isEmpty()) {
    bool saveResult = qglview->save(img_filename.toLocal8Bit().constData());
    if (saveResult) {
      this->export_paths[suffix] = img_filename;
      setCurrentOutput();
      fileExportedMessage("PNG", img_filename);
      clearCurrentOutput();
    } else {
      LOG("Can't open file \"%1$s\" for export image", img_filename.toLocal8Bit().constData());
    }
  }
}
      break;
    default:
      actionExport(info.format, info.description.c_str(), suffix.c_str(), fileformat::is3D(format) ? 3 : fileformat::is2D(format) ? 2 : 0);
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
  viewActionSurfaces->setChecked(false);
  viewActionWireframe->setChecked(false);
  viewActionThrownTogether->setChecked(false);
}

#ifdef ENABLE_OPENCSG

/*!
   Go to the OpenCSG view mode.
   Falls back to thrown together mode if OpenCSG is not available
 */
void MainWindow::viewModePreview()
{
  if (this->qglview->hasOpenCSGSupport()) {
    viewModeActionsUncheck();
    viewActionPreview->setChecked(true);
    this->qglview->setRenderer(this->opencsgRenderer ? this->opencsgRenderer : this->thrownTogetherRenderer);
    this->qglview->updateColorScheme();
    this->qglview->update();
  } else {
    viewModeThrownTogether();
  }
}

#endif /* ENABLE_OPENCSG */

void MainWindow::viewModeSurface()
{
  viewModeActionsUncheck();
  viewActionSurfaces->setChecked(true);
  this->qglview->setShowFaces(true);
  this->qglview->setRenderer(this->cgalRenderer);
  this->qglview->updateColorScheme();
  this->qglview->update();
}

void MainWindow::viewModeWireframe()
{
  viewModeActionsUncheck();
  viewActionWireframe->setChecked(true);
  this->qglview->setShowFaces(false);
  this->qglview->setRenderer(this->cgalRenderer);
  this->qglview->updateColorScheme();
  this->qglview->update();
}

void MainWindow::viewModeThrownTogether()
{
  viewModeActionsUncheck();
  viewActionThrownTogether->setChecked(true);
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
  bool showaxes = viewActionShowAxes->isChecked();
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

bool MainWindow::isEmpty()
{
  return activeEditor->toPlainText().isEmpty();
}

void MainWindow::editorContentChanged()
{
  auto current_doc = activeEditor->toPlainText();
  if (current_doc != last_compiled_doc) {
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

void MainWindow::viewPerspective()
{
  QSettingsCached settings;
  settings.setValue("view/orthogonalProjection", false);
  viewActionPerspective->setChecked(true);
  viewActionOrthogonal->setChecked(false);
  this->qglview->setOrthoMode(false);
  this->qglview->update();
}

void MainWindow::viewOrthogonal()
{
  QSettingsCached settings;
  settings.setValue("view/orthogonalProjection", true);
  viewActionPerspective->setChecked(false);
  viewActionOrthogonal->setChecked(true);
  this->qglview->setOrthoMode(true);
  this->qglview->update();
}

void MainWindow::viewTogglePerspective()
{
  QSettingsCached settings;
  if (settings.value("view/orthogonalProjection").toBool()) {
    viewPerspective();
  } else {
    viewOrthogonal();
  }
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

void MainWindow::on_editorDock_visibilityChanged(bool)
{
  changedTopLevelEditor(editorDock->isFloating());
  tabToolBar->setVisible((tabCount > 1) && editorDock->isVisible());
  updateExportActions();
}

void MainWindow::on_consoleDock_visibilityChanged(bool)
{
  changedTopLevelConsole(consoleDock->isFloating());
}

void MainWindow::on_parameterDock_visibilityChanged(bool)
{
  parameterTopLevelChanged(parameterDock->isFloating());
}

void MainWindow::on_errorLogDock_visibilityChanged(bool)
{
  errorLogTopLevelChanged(errorLogDock->isFloating());
}

void MainWindow::on_animateDock_visibilityChanged(bool)
{
  animateTopLevelChanged(animateDock->isFloating());
}

void MainWindow::on_fontListDock_visibilityChanged(bool)
{
  fontListTopLevelChanged(fontListDock->isFloating());
}

void MainWindow::on_viewportControlDock_visibilityChanged(bool)
{
  viewportControlTopLevelChanged(viewportControlDock->isFloating());
}

void MainWindow::changedTopLevelEditor(bool topLevel)
{
  setDockWidgetTitle(editorDock, QString(_("Editor")), topLevel);
}

void MainWindow::editorTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(editorDock, QString(_("Editor")), topLevel);
  if (topLevel) {
    this->removeToolBar(tabToolBar);
    ((QVBoxLayout *)editorDockContents->layout())->insertWidget(0, tabToolBar);
  } else {
    editorDockContents->layout()->removeWidget(tabToolBar);
    this->addToolBar(tabToolBar);
  }
  tabToolBar->setVisible((tabCount > 1) && editorDock->isVisible());
}

void MainWindow::changedTopLevelConsole(bool topLevel)
{
  setDockWidgetTitle(consoleDock, QString(_("Console")), topLevel);
}

void MainWindow::consoleTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(consoleDock, QString(_("Console")), topLevel);

  Qt::WindowFlags flags = (consoleDock->windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (topLevel) {
    consoleDock->setWindowFlags(flags);
    consoleDock->show();
  }
}

void MainWindow::parameterTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(parameterDock, QString(_("Customizer")), topLevel);
}

void MainWindow::changedTopLevelErrorLog(bool topLevel)
{
  setDockWidgetTitle(errorLogDock, QString(_("Error-Log")), topLevel);
}

void MainWindow::errorLogTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(errorLogDock, QString(_("Error-Log")), topLevel);

  Qt::WindowFlags flags = (errorLogDock->windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (topLevel) {
    errorLogDock->setWindowFlags(flags);
    errorLogDock->show();
  }
}

void MainWindow::changedTopLevelAnimate(bool topLevel)
{
  setDockWidgetTitle(animateDock, QString(_("Animate")), topLevel);
}

void MainWindow::animateTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(animateDock, QString(_("Animate")), topLevel);

  Qt::WindowFlags flags = (animateDock->windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (topLevel) {
    animateDock->setWindowFlags(flags);
    animateDock->show();
  }
}

void MainWindow::changedTopLevelFontList(bool topLevel)
{
  setDockWidgetTitle(fontListDock, QString(_("Font List")), topLevel);
}

void MainWindow::fontListTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(fontListDock, QString(_("Font List")), topLevel);

  Qt::WindowFlags flags = (fontListDock->windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (topLevel) {
    fontListDock->setWindowFlags(flags);
    fontListDock->show();
  }
}

void MainWindow::viewportControlTopLevelChanged(bool topLevel)
{
  setDockWidgetTitle(viewportControlDock, QString(_("Viewport-Control")), topLevel);

  Qt::WindowFlags flags = (viewportControlDock->windowFlags() & ~Qt::WindowType_Mask) | Qt::Window;
  if (topLevel) {
    viewportControlDock->setWindowFlags(flags);
    viewportControlDock->show();
  }
}

void MainWindow::setDockWidgetTitle(QDockWidget *dockWidget, QString prefix, bool topLevel)
{
  QString title(std::move(prefix));
  if (topLevel) {
    const QFileInfo fileInfo(activeEditor->filepath);
    QString fname = _("Untitled.scad");
    if (!fileInfo.fileName().isEmpty()) fname = fileInfo.fileName();
    title += " (" + fname.replace("&", "&&") + ")";
  }
  dockWidget->setWindowTitle(title);
}

void MainWindow::hideEditorToolbar()
{
  QSettingsCached settings;
  bool shouldHide = viewActionHideEditorToolBar->isChecked();
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
  bool shouldHide = viewActionHide3DViewToolBar->isChecked();
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
    showConsole();
  } else if (link == "#errorlog") {
    showErrorLog();
  }
}

void MainWindow::showEditor()
{
  windowActionHideEditor->setChecked(false);
  hideEditor();
  editorDock->raise();
  tabManager->setFocus();
}

void MainWindow::hideEditor()
{
  auto e = (ScintillaEditor *) this->activeEditor;
  if (windowActionHideEditor->isChecked()) {
    // Workaround manually disabling interactions with editor by setting it
    // to read-only when not being shown.  This is an upstream bug from Qt
    // (tracking ticket: https://bugreports.qt.io/browse/QTBUG-82939) and
    // may eventually get resolved at which point this bit and the stuff in
    // the else should be removed. Currently known to affect 5.14.1 and 5.15.0
    e->qsci->setReadOnly(true);
    e->setupAutoComplete(true);
    editorDock->close();
  } else {
    e->qsci->setReadOnly(false);
    e->setupAutoComplete(false);
    editorDock->show();
  }
}

void MainWindow::showConsole()
{
  windowActionHideConsole->setChecked(false);
  frameCompileResult->hide();
  consoleDock->show();
  consoleDock->raise();
  console->setFocus();
}

void MainWindow::hideConsole()
{
  if (windowActionHideConsole->isChecked()) {
    consoleDock->hide();
  } else {
    consoleDock->show();
  }
}

void MainWindow::showErrorLog()
{
  windowActionHideErrorLog->setChecked(false);
  frameCompileResult->hide();
  errorLogDock->show();
  errorLogDock->raise();
  errorLogWidget->logTable->setFocus();
}

void MainWindow::hideErrorLog()
{
  if (windowActionHideErrorLog->isChecked()) {
    errorLogDock->hide();
  } else {
    errorLogDock->show();
  }
}

void MainWindow::showAnimate()
{
  windowActionHideAnimate->setChecked(false);
  animateDock->show();
  animateDock->raise();
  animateWidget->setFocus();
}

void MainWindow::hideAnimate()
{
  if (windowActionHideAnimate->isChecked()) {
    animateDock->hide();
  } else {
    animateDock->show();
  }
}

void MainWindow::showFontList()
{
  windowActionHideFontList->setChecked(false);
  fontListWidget->update_font_list();
  fontListDock->show();
  fontListDock->raise();
  fontListWidget->setFocus();
}

void MainWindow::hideFontList()
{
  if (windowActionHideFontList->isChecked()) {
    fontListDock->hide();
  } else {
    fontListWidget->update_font_list();
    fontListDock->show();
  }
}

void MainWindow::showViewportControl()
{
  windowActionHideViewportControl->setChecked(false);
  viewportControlDock->show();
  viewportControlDock->raise();
  viewportControlWidget->setFocus();
}

void MainWindow::hideViewportControl()
{
  if (windowActionHideViewportControl->isChecked()) {
    viewportControlDock->hide();
  } else {
    viewportControlDock->show();
  }
}


void MainWindow::showParameters()
{
  windowActionHideCustomizer->setChecked(false);
  parameterDock->show();
  parameterDock->raise();
  activeEditor->parameterWidget->scrollArea->setFocus();
}

void MainWindow::hideParameters()
{
  if (windowActionHideCustomizer->isChecked()) {
    parameterDock->hide();
  } else {
    parameterDock->show();
  }
}

void MainWindow::on_windowActionSelectEditor_triggered()
{
  showEditor();
}

void MainWindow::on_windowActionSelectConsole_triggered()
{
  showConsole();
}

void MainWindow::on_windowActionSelectErrorLog_triggered()
{
  showErrorLog();
}

void MainWindow::on_windowActionSelectAnimate_triggered()
{
  showAnimate();
}

void MainWindow::on_windowActionSelectFontList_triggered()
{
  showFontList();
}

void MainWindow::on_windowActionSelectViewportControl_triggered()
{
  showViewportControl();
}

void MainWindow::on_windowActionSelectCustomizer_triggered()
{
  showParameters();
}

void MainWindow::on_windowActionNextWindow_triggered()
{
  activateWindow(1);
}

void MainWindow::on_windowActionPreviousWindow_triggered()
{
  activateWindow(-1);
}

void MainWindow::on_editActionInsertTemplate_triggered()
{
  activeEditor->displayTemplates();
}

void MainWindow::on_editActionFoldAll_triggered()
{
  activeEditor->foldUnfold();
}

void MainWindow::activateWindow(int offset)
{
  const std::array<DockFocus, 7> docks = {{
    { editorDock, &MainWindow::on_windowActionSelectEditor_triggered },
    { consoleDock, &MainWindow::on_windowActionSelectConsole_triggered },
    { errorLogDock, &MainWindow::on_windowActionSelectErrorLog_triggered },
    { parameterDock, &MainWindow::on_windowActionSelectCustomizer_triggered },
    { fontListDock, &MainWindow::on_windowActionSelectFontList_triggered },
    { animateDock, &MainWindow::on_windowActionSelectAnimate_triggered },
    { viewportControlDock, &MainWindow::on_windowActionSelectViewportControl_triggered },
  }};

  const int cnt = docks.size();
  const auto focusWidget = QApplication::focusWidget();
  for (auto widget = focusWidget; widget != nullptr; widget = widget->parentWidget()) {
    for (int idx = 0; idx < cnt; ++idx) {
      if (widget == docks.at(idx).widget) {
        for (int o = 1; o < cnt; ++o) {
          const int target = (cnt + idx + o * offset) % cnt;
          const auto& dock = docks.at(target);
          if (dock.widget->isVisible()) {
            dock.focus(this);
            return;
          }
        }
      }
    }
  }
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
  const auto cmd = knownFileExtensions[suffix];
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

void MainWindow::helpHomepage()
{
  UIUtils::openHomepageURL();
}

void MainWindow::helpManual()
{
  UIUtils::openUserManualURL();
}

void MainWindow::helpOfflineManual()
{
  UIUtils::openOfflineUserManual();
}

void MainWindow::helpCheatSheet()
{
  UIUtils::openCheatSheetURL();
}

void MainWindow::helpOfflineCheatSheet()
{
  UIUtils::openOfflineCheatSheet();
}

void MainWindow::helpLibrary()
{
  if (!this->library_info_dialog) {
    QString rendererInfo(qglview->getRendererInfo().c_str());
    auto dialog = new LibraryInfoDialog(rendererInfo);
    this->library_info_dialog = dialog;
  }
  this->library_info_dialog->show();
}

void MainWindow::helpFontInfo()
{
  if (!this->font_list_dialog) {
    auto dialog = new FontListDialog();
    this->font_list_dialog = dialog;
  }
  this->font_list_dialog->update_font_list();
  this->font_list_dialog->show();
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
  Preferences::inst()->show();
  Preferences::inst()->activateWindow();
  Preferences::inst()->raise();
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

void MainWindow::quit()
{
  QCloseEvent ev;
  QApplication::sendEvent(QApplication::instance(), &ev);
  if (ev.isAccepted()) QApplication::instance()->quit();
  // FIXME: Cancel any CGAL calculations
#ifdef Q_OS_MACOS
  CocoaUtils::endApplication();
#endif
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
    consoleUpdater->start(50); // Limit console updates to 20 FPS
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

void MainWindow::errorLogOutput(const Message& log_msg)
{
  this->errorLogWidget->toErrorLog(log_msg);
}

void MainWindow::setCurrentOutput()
{
  set_output_handler(&MainWindow::consoleOutput, &MainWindow::errorLogOutput, this);
}

void MainWindow::hideCurrentOutput()
{
  set_output_handler(&MainWindow::noOutputConsole, &MainWindow::noOutputErrorLog, this);
}

void MainWindow::clearCurrentOutput()
{
  set_output_handler(nullptr, nullptr, nullptr);
}

void MainWindow::openCSGSettingsChanged()
{
#ifdef ENABLE_OPENCSG
  OpenCSG::setOption(OpenCSG::AlgorithmSetting, Preferences::inst()->getValue("advanced/forceGoldfeather").toBool() ?
                     OpenCSG::Goldfeather : OpenCSG::Automatic);
#endif
}

void MainWindow::processEvents()
{
  if (this->procevents) QApplication::processEvents();
}

QString MainWindow::exportPath(const char *suffix) {
  QString path;
  auto path_it = this->export_paths.find(suffix);
  if (path_it != export_paths.end()) {
    path = QFileInfo(path_it->second).absolutePath() + QString("/");
    if (activeEditor->filepath.isEmpty()) path += QString(_("Untitled")) + suffix;
    else path += QFileInfo(activeEditor->filepath).completeBaseName() + suffix;
  } else {
    if (activeEditor->filepath.isEmpty()) path = QString(PlatformUtils::userDocumentsPath().c_str()) + QString("/") + QString(_("Untitled")) + suffix;
    else {
      auto info = QFileInfo(activeEditor->filepath);
      path = info.absolutePath() + QString("/") + info.completeBaseName() + suffix;
    }
  }
  return path;
}

void MainWindow::jumpToLine(int line, int col)
{
  this->activeEditor->setCursorPosition(line, col);
}

paperSizes MainWindow::sizeString2Enum(const QString& current){
   for(size_t i = 0; i < paperSizeStrings.size(); i++){
       if (current.toStdString()==paperSizeStrings[i]) return static_cast<paperSizes>(i);
   };
   return paperSizes::A4;
};

paperOrientations MainWindow::orientationsString2Enum(const QString& current){
   for(size_t i = 0; i < paperOrientationsStrings.size(); i++){
       if (current.toStdString()==paperOrientationsStrings[i]) return static_cast<paperOrientations>(i);
   };
   return paperOrientations::PORTRAIT;
};
