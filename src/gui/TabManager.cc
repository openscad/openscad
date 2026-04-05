#include "gui/TabManager.h"

#include <QApplication>
#include <QStringBuilder>
#include <string>
#include <tuple>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QKeyCombination>
#include <QStringConverter>
#endif
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <Qsci/qscicommand.h>
#include <Qsci/qscicommandset.h>

#include <QByteArray>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPoint>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStringList>
#include <QTabBar>
#include <QTextStream>
#include <QWidget>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <sstream>
#include <vector>

#include "gui/Editor.h"
#include "gui/ImportUtils.h"
#include <boost/algorithm/string/predicate.hpp>
#include "gui/MainWindow.h"
#include "gui/OpenSCADApp.h"
#include "gui/Preferences.h"
#include "gui/ScintillaEditor.h"
#include "gui/UnsavedChangesDialog.h"
#include "utils/printutils.h"
#include <genlang/genlang.h>

#include <algorithm>

namespace {
uint64_t sessionDirtyGenerationValue = 0;
bool sessionSaveWarningShown = false;
bool skipSessionSaveOnQuit = false;

QJsonArray vec3ToJson(const Eigen::Vector3d& vec)
{
  QJsonArray arr;
  arr.append(vec.x());
  arr.append(vec.y());
  arr.append(vec.z());
  return arr;
}

bool jsonToVec3(const QJsonValue& value, double *x, double *y, double *z)
{
  const QJsonArray arr = value.toArray();
  if (arr.size() != 3 || !x || !y || !z) return false;
  *x = arr[0].toDouble();
  *y = arr[1].toDouble();
  *z = arr[2].toDouble();
  return true;
}

void initEmptyUntitledTab(EditorInterface *editor)
{
#ifdef ENABLE_PYTHON
  std::string templ = "from openscad import *\n";
  std::string libs = Settings::SettingsPython::pythonNetworkImportList.value();
  std::stringstream ss(libs);
  std::string word;
  while (std::getline(ss, word, '\n')) {
    if (word.size() == 0) continue;
    templ += "nimport(\"" + word + "\")\n";
  }
  editor->setPlainText(QString::fromStdString(templ));
  editor->setLanguageManually(LANG_PYTHON);
#else
  editor->setPlainText("");
#endif
}

void warnSessionSaveFailure(const QString& path, const QString& error)
{
  if (sessionSaveWarningShown) return;
  sessionSaveWarningShown = true;

  QWidget *parent = nullptr;
  if (scadApp && !scadApp->windowManager.getWindows().isEmpty()) {
    parent = *scadApp->windowManager.getWindows().begin();
  }
  QMessageBox::warning(
    parent, QString(_("Session Save")),
    QString(_("Could not write session file:\n%1\n\n%2\n\nSession changes may be lost."))
      .arg(path, error));
}

bool writeSessionFile(const QJsonObject& root, const QString& path, QString *error)
{
  const QFileInfo pathInfo(path);
  const QDir dir = pathInfo.absoluteDir();
  if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
    if (error) *error = QString(_("Unable to create the session directory."));
    return false;
  }

  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    if (error) *error = file.errorString();
    return false;
  }

  const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Compact);
  if (file.write(json) != json.size()) {
    file.cancelWriting();
    if (error) *error = file.errorString();
    return false;
  }

  if (!file.commit()) {
    if (error) *error = file.errorString();
    return false;
  }
  if (!QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
    LOG(message_group::UI_Warning, "Failed to set session file permissions: %1$s",
        path.toLocal8Bit().constData());
  }
  return true;
}
}  // namespace

TabManager::TabManager(MainWindow *o, const QString& filename)
{
  parent = o;

  tabWidget = new QTabWidget();
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);
  tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(tabWidget, &QTabWidget::tabCloseRequested, this, &TabManager::closeTabRequested);
  connect(tabWidget, &QTabWidget::customContextMenuRequested, this,
          &TabManager::showTabHeaderContextMenu);

  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::stopAnimation);
  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::updateFindState);
  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::tabSwitched);

  connect(parent->editActionZoomTextIn, &QAction::triggered, this, &TabManager::zoomIn);
  connect(parent->editActionZoomTextOut, &QAction::triggered, this, &TabManager::zoomOut);

  createTab(filename);

  // Disable the closing button for the first tabbar
  setTabsCloseButtonVisibility(0, false);
}

QTabBar::ButtonPosition TabManager::getClosingButtonPosition()
{
  auto bar = tabWidget->tabBar();
  return (QTabBar::ButtonPosition)bar->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr,
                                                          bar);
}

void TabManager::setTabsCloseButtonVisibility(int indice, bool isVisible)
{
  // Depending on the system the closing button can be on the right or left side
  // of the tab header.
  auto button = tabWidget->tabBar()->tabButton(indice, getClosingButtonPosition());
  if (button) button->setVisible(isVisible);
}

QWidget *TabManager::getTabContent()
{
  assert(tabWidget != nullptr);
  return tabWidget;
}

void TabManager::tabSwitched(int x)
{
  assert(tabWidget != nullptr);

  editor = (EditorInterface *)tabWidget->widget(x);
  parent->activeEditor = editor;
  parent->parameterDock->setWidget(editor->parameterWidget);

  parent->editActionUndo->setEnabled(editor->canUndo());
  parent->setWindowTitle(tabWidget->tabText(x).replace("&&", "&"));
  if (use_gvim) {
    // **MCH*
    auto *tabEditor = (EditorInterface *)tabWidget->widget(x);
    std::string filename = tabEditor->filepath.toUtf8().constData();
    QString editorcmd = "gvim --remote-send '<esc>:sb " + QString::fromStdString(filename) +
                        "<cr>' || (gvim '" + QString::fromStdString(filename) + "' &)";
    //   LOG("1. Opening file '%1$s'",editorcmd.toUtf8().constData());
    system(editorcmd.toUtf8().constData());
    // **MCH*
  }

  auto numberOfOpenTabs = tabWidget->count();
  // Hides all the closing button except the one on the currently focused editor
  for (int idx = 0; idx < numberOfOpenTabs; ++idx) {
    bool isVisible = idx == x && numberOfOpenTabs > 1;
    setTabsCloseButtonVisibility(idx, isVisible);
  }

  editor->recomputeLanguageActive();
  parent->onLanguageActiveChanged(editor->language);
  emit currentEditorChanged(editor);
}

void TabManager::closeTabRequested(int x)
{
  assert(tabWidget != nullptr);
  if (!maybeSave(x)) return;

  auto *closingEditor = qobject_cast<EditorInterface *>(tabWidget->widget(x));
  assert(closingEditor != nullptr);

  emit editorAboutToClose(closingEditor);

  editorList.remove(closingEditor);
  tabWidget->removeTab(x);
  emit tabCountChanged(editorList.size());

  delete closingEditor->parameterWidget;
  delete closingEditor;
}

void TabManager::closeCurrentTab()
{
  assert(tabWidget != nullptr);

  /* Close tab or close the current window if only one tab is open. */
  if (tabWidget->count() > 1) this->closeTabRequested(tabWidget->currentIndex());
  else {
    parent->close();
    if (use_gvim) this->closeTabRequested(tabWidget->currentIndex());  // ** MCH **
  }
}

void TabManager::nextTab()
{
  assert(tabWidget != nullptr);
  tabWidget->setCurrentIndex((tabWidget->currentIndex() + 1) % tabWidget->count());
}

void TabManager::prevTab()
{
  assert(tabWidget != nullptr);
  tabWidget->setCurrentIndex((tabWidget->currentIndex() + tabWidget->count() - 1) % tabWidget->count());
}

void TabManager::switchToEditor(EditorInterface *targetEditor)
{
  assert(tabWidget != nullptr);
  int index = tabWidget->indexOf(targetEditor);
  if (index >= 0) {
    tabWidget->setCurrentIndex(index);
  }
}

void TabManager::actionNew()
{
  if (!parent->editorDock->isVisible())
    parent->editorDock->setVisible(true);  // if editor hidden, make it visible
  createTab(QString());
}

void TabManager::open(const QString& filename)
{
  assert(!filename.isEmpty());

  if (use_gvim) {
    QString editorcmd =
      "gvim --remote-tab-silent '" + filename.toUtf8() + "' || gvim '" + filename.toUtf8() + "' &";
    editorcmd += filename.toUtf8();
    //    LOG("2. Opening file '%1$s'",editorcmd.toUtf8().constData());
    system(editorcmd.toUtf8().constData());
  }
  for (auto edt : editorList) {
    if (filename == edt->filepath) {
      tabWidget->setCurrentWidget(edt);
      return;
    }
  }

  if (editor->filepath.isEmpty() && !editor->isContentModified() &&
      !editor->parameterWidget->isModified()) {
    // Empty tabs from "New" may be setLanguageManually(LANG_PYTHON); clear that before loading
    // so recomputeLanguageActive() follows the opened file's extension (.scad vs .py).
    editor->resetLanguageDetection();
    openTabFile(filename);
    editor->recomputeLanguageActive();
    parent->onLanguageActiveChanged(editor->language);
    updateTabIcon(editor);
    emit editorContentReloaded(editor);
  } else {
    createTab(filename);
  }
}

void TabManager::createTab(const QString& filename, bool initializeEmptyEditor)
{
  assert(parent != nullptr);

  auto scintillaEditor = new ScintillaEditor(tabWidget);
  editor = scintillaEditor;
  //  Preferences::create(editor->colorSchemes());   // needs to be done only once, however handled
  this->use_gvim = GlobalPreferences::inst()->getValue("editor/usegvim").toBool();
  //  this->use_gvim = true;
  parent->activeEditor = editor;
  editor->parameterWidget = new ParameterWidget(parent->parameterDock);
  connect(editor->parameterWidget, &ParameterWidget::parametersChanged, parent,
          &MainWindow::actionRenderPreview);
  parent->parameterDock->setWidget(editor->parameterWidget);

  // clearing default mapping of keyboard shortcut for font size
  QsciCommandSet *qcmdset = scintillaEditor->qsci->standardCommands();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QsciCommand *qcmd = qcmdset->boundTo((Qt::ControlModifier | Qt::Key_Plus).toCombined());
  qcmd->setKey(0);
  qcmd = qcmdset->boundTo((Qt::ControlModifier | Qt::Key_Minus).toCombined());
  qcmd->setKey(0);
#else
  QsciCommand *qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Plus);
  qcmd->setKey(0);
  qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Minus);
  qcmd->setKey(0);
#endif

  connect(scintillaEditor, &ScintillaEditor::uriDropped, parent, &MainWindow::handleFileDrop);
  connect(scintillaEditor, &ScintillaEditor::previewRequest, parent, &MainWindow::actionRenderPreview);
  connect(editor, &EditorInterface::showContextMenuEvent, this, &TabManager::showContextMenuEvent);
  connect(editor, &EditorInterface::focusIn, this, [this]() { parent->setLastFocus(editor); });

  connect(GlobalPreferences::inst(), &Preferences::editorConfigChanged, scintillaEditor,
          &ScintillaEditor::applySettings);
  connect(GlobalPreferences::inst(), &Preferences::autocompleteChanged, scintillaEditor,
          &ScintillaEditor::onAutocompleteChanged);
  connect(GlobalPreferences::inst(), &Preferences::characterThresholdChanged, scintillaEditor,
          &ScintillaEditor::onCharacterThresholdChanged);
  scintillaEditor->applySettings();
  editor->addTemplate();

  connect(editor, &EditorInterface::contentsChanged, this, &TabManager::updateActionUndoState);
  connect(editor, &EditorInterface::contentsChanged, parent, &MainWindow::editorContentChanged);
  connect(editor, &EditorInterface::contentsChanged, this, &TabManager::setContentRenderState);
  // Bump autosave generation on each edit while dirty. modificationChanged only fires when the
  // modified flag toggles, so typing in an already-dirty tab would otherwise not advance
  // sessionDirtyGeneration() and periodic autosave could skip indefinitely (Copilot PR #415).
  connect(editor, &EditorInterface::contentsChanged, this, [edt = scintillaEditor]() {
    if (edt->isContentModified()) {
      TabManager::bumpSessionDirtyGeneration();
    }
  });
  connect(editor, &EditorInterface::modificationChanged, this, &TabManager::onTabModified);
  connect(editor->parameterWidget, &ParameterWidget::modificationChanged,
          [editor = this->editor, this] { onTabModified(editor); });

  connect(GlobalPreferences::inst(), &Preferences::fontChanged, editor, &EditorInterface::initFont);
  connect(GlobalPreferences::inst(), &Preferences::syntaxHighlightChanged, editor,
          &EditorInterface::setHighlightScheme);
  editor->initFont(GlobalPreferences::inst()->getValue("editor/fontfamily").toString(),
                   GlobalPreferences::inst()->getValue("editor/fontsize").toUInt());
  editor->setHighlightScheme(GlobalPreferences::inst()->getValue("editor/syntaxhighlight").toString());

  connect(scintillaEditor, &ScintillaEditor::hyperlinkIndicatorClicked, this,
          &TabManager::onHyperlinkIndicatorClicked);

  // Fill the editor with the content of the file
  if (filename.isEmpty()) {
    editor->filepath = "";
    if (initializeEmptyEditor) {
      initEmptyUntitledTab(editor);
      refreshDocument();
    }
  } else {
    openTabFile(filename);
  }
  editorList.insert(editor);

  // Get the name of the tab in editor
  auto [fname, fpath] = getEditorTabNameWithModifier(editor);
  tabWidget->addTab(editor, fname);
  if (tabWidget->currentWidget() != editor) {
    tabWidget->setCurrentWidget(editor);
  }

  editor->recomputeLanguageActive();
  parent->onLanguageActiveChanged(editor->language);
  updateTabIcon(editor);

  emit tabCountChanged(editorList.size());
}

size_t TabManager::count()
{
  return tabWidget->count();
}

void TabManager::highlightError(int i)
{
  editor->highlightError(i);
}

void TabManager::unhighlightLastError()
{
  editor->unhighlightLastError();
}

void TabManager::undo()
{
  editor->undo();
}

void TabManager::redo()
{
  editor->redo();
}

void TabManager::cut()
{
  editor->cut();
}

void TabManager::copy()
{
  editor->copy();
}

void TabManager::paste()
{
  editor->paste();
}

void TabManager::indentSelection()
{
  editor->indentSelection();
}

void TabManager::unindentSelection()
{
  editor->unindentSelection();
}

void TabManager::commentSelection()
{
  editor->commentSelection();
}

void TabManager::uncommentSelection()
{
  editor->uncommentSelection();
}

void TabManager::toggleBookmark()
{
  editor->toggleBookmark();
}

void TabManager::nextBookmark()
{
  editor->nextBookmark();
}

void TabManager::prevBookmark()
{
  editor->prevBookmark();
}

void TabManager::jumpToNextError()
{
  editor->jumpToNextError();
}

void TabManager::setFocus()
{
  editor->setFocus();
}

void TabManager::updateActionUndoState()
{
  parent->editActionUndo->setEnabled(editor->canUndo());
}

void TabManager::onHyperlinkIndicatorClicked(int val)
{
  const QString filename = QString::fromStdString(editor->indicatorData[val].path);
  this->open(filename);
}

void TabManager::applyAction(QObject *object, const std::function<void(int, EditorInterface *)>& func)
{
  auto *action = dynamic_cast<QAction *>(object);
  if (action == nullptr) {
    return;
  }
  bool ok;
  int idx = action->data().toInt(&ok);
  if (!ok) {
    return;
  }

  auto *edt = (EditorInterface *)tabWidget->widget(idx);
  if (edt == nullptr) {
    return;
  }

  func(idx, edt);
}

void TabManager::copyFileName()
{
  applyAction(QObject::sender(), [](int, EditorInterface *edt) {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QFileInfo(edt->filepath).fileName());
  });
}

void TabManager::copyFilePath()
{
  applyAction(QObject::sender(), [](int, EditorInterface *edt) {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(edt->filepath);
  });
}

void TabManager::openFolder()
{
  applyAction(QObject::sender(), [](int, EditorInterface *edt) {
    auto dir = QFileInfo(edt->filepath).dir();
    if (dir.exists()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
    }
  });
}

void TabManager::closeTab()
{
  applyAction(QObject::sender(), [this](int idx, EditorInterface *) { closeTabRequested(idx); });
}

void TabManager::closeAllButThisTab()
{
  applyAction(QObject::sender(), [this](int idx, EditorInterface *) {
    int total = count();
    for (int i = total - 1; i >= 0; i--) {
      if (i != idx) closeTabRequested(i);
    }
  });
}

void TabManager::showContextMenuEvent(const QPoint& pos)
{
  auto menu = editor->createStandardContextMenu();

  menu->addSeparator();
  menu->addAction(parent->editActionFind);
  menu->addAction(parent->editActionFindNext);
  menu->addAction(parent->editActionFindPrevious);
  menu->addSeparator();
  menu->addAction(parent->editActionInsertTemplate);
  menu->addAction(parent->editActionFoldAll);
  menu->exec(editor->mapToGlobal(pos));

  delete menu;
}

void TabManager::showTabHeaderContextMenu(const QPoint& pos)
{
  int idx = tabWidget->tabBar()->tabAt(pos);
  if (idx < 0) return;

  QMenu menu;
  auto *edt = (EditorInterface *)tabWidget->widget(idx);

  auto *copyFileNameAction = new QAction(tabWidget);
  copyFileNameAction->setData(idx);
  copyFileNameAction->setEnabled(!edt->filepath.isEmpty());
  copyFileNameAction->setText(_("Copy file name"));
  connect(copyFileNameAction, &QAction::triggered, this, &TabManager::copyFileName);

  auto *copyFilePathAction = new QAction(tabWidget);
  copyFilePathAction->setData(idx);
  copyFilePathAction->setEnabled(!edt->filepath.isEmpty());
  copyFilePathAction->setText(_("Copy full path"));
  connect(copyFilePathAction, &QAction::triggered, this, &TabManager::copyFilePath);

  auto *openFolderAction = new QAction(tabWidget);
  openFolderAction->setData(idx);
  openFolderAction->setEnabled(!edt->filepath.isEmpty());
  openFolderAction->setText(_("Open Folder"));
  connect(openFolderAction, &QAction::triggered, this, &TabManager::openFolder);

  auto *closeAction = new QAction(tabWidget);
  closeAction->setData(idx);
  closeAction->setText(_("Close Tab"));
  connect(closeAction, &QAction::triggered, this, &TabManager::closeTab);

  auto *closeAllButThisAction = new QAction(tabWidget);
  closeAllButThisAction->setData(idx);
  closeAllButThisAction->setText(_("Close All But This Tab"));
  connect(closeAllButThisAction, &QAction::triggered, this, &TabManager::closeAllButThisTab);

  // Don't allow to close the last tab.
  if (tabWidget->count() <= 1) {
    closeAction->setDisabled(true);
    closeAllButThisAction->setDisabled(true);
  }

  menu.addAction(copyFileNameAction);
  menu.addAction(copyFilePathAction);
  menu.addSeparator();
  menu.addAction(openFolderAction);
  menu.addSeparator();
  menu.addAction(closeAction);
  menu.addAction(closeAllButThisAction);

  QPoint globalCursorPos = QCursor::pos();
  menu.exec(globalCursorPos);
}

void TabManager::setContentRenderState()  // since last render
{
  editor->contentsRendered = false;  // since last render
  editor->parameterWidget->setEnabled(false);
}

void TabManager::stopAnimation()
{
  parent->animateWidget->pauseAnimation();
  parent->animateWidget->e_tval->setText("");
}

void TabManager::updateFindState()
{
  if (editor->findState == TabManager::FIND_REPLACE_VISIBLE) parent->showFind(true);
  else if (editor->findState == TabManager::FIND_VISIBLE) parent->showFind(false);
  else parent->hideFind();
}

void TabManager::onTabModified(EditorInterface *edt)
{
  bumpSessionDirtyGeneration();
  // Get the name of the editor and its filepath with the status modifier
  auto [fname, fpath] = getEditorTabNameWithModifier(edt);

  // and set the tab bar widget.
  setEditorTabName(fname, fpath, edt);
}

void TabManager::openTabFile(const QString& filename)
{
  QFileInfo fileinfo(filename);
  const QString absPath = fileinfo.absoluteFilePath();
  const QString suffix = Importer::effectiveSuffixForOpen(filename);
  if (!Importer::knownFileExtensions.contains(suffix)) {
    editor->setPlainText("");
    if (!fileinfo.exists() || !fileinfo.isFile()) {
      return;
    }
    editor->filepath = absPath;
    editor->resetLanguageDetection();
    editor->parameterWidget->resetForNewDocument();
    editor->parameterWidget->readFile(absPath);
    parent->updateRecentFiles(filename);
    refreshDocument();
    editor->recomputeLanguageActive();
    parent->onLanguageActiveChanged(editor->language);
    updateTabIcon(editor);
    auto [fname, fpath] = getEditorTabNameWithModifier(editor);
    setEditorTabName(fname, fpath, editor);
    parent->setWindowTitle(fname);
    emit editorContentReloaded(editor);
    return;
  }
#ifdef ENABLE_PYTHON
  if (suffix == QStringLiteral("py")) {
    std::string templ = "from openscad import *\n";
    std::string libs = Settings::SettingsPython::pythonNetworkImportList.value();
    std::stringstream ss(libs);
    std::string word;
    while (std::getline(ss, word, '\n')) {
      if (word.size() == 0) continue;
      templ += "nimport(\"" + word + "\")\n";
    }
    editor->setPlainText(QString::fromStdString(templ));
  } else
#endif
    editor->setPlainText("");

  const auto cmd = Importer::knownFileExtensions[suffix];
  if (cmd.isEmpty()) {
    editor->filepath = fileinfo.absoluteFilePath();
#ifdef ENABLE_PYTHON
    if (suffix == QStringLiteral("py")) {
      const QByteArray pathUtf8 = editor->filepath.toUtf8();
      parent->clearPythonUntrustStateForPath(
        std::string(pathUtf8.constData(), static_cast<size_t>(pathUtf8.size())));
    }
#endif
    editor->parameterWidget->resetForNewDocument();
    editor->parameterWidget->readFile(fileinfo.absoluteFilePath());
    parent->updateRecentFiles(filename);
  } else {
    editor->filepath.clear();
    editor->language = LANG_PYTHON;
    editor->languageManuallySet = true;
    editor->setPlainText(cmd.arg(filename));
  }
  refreshDocument();

  auto [fname, fpath] = getEditorTabNameWithModifier(editor);
  setEditorTabName(fname, fpath, editor);
  parent->setWindowTitle(fname);
  emit editorContentReloaded(editor);
}

std::tuple<QString, QString> TabManager::getEditorTabName(EditorInterface *edt)
{
  QString fname = edt->filepath;
  QString fpath = edt->filepath;
  QFileInfo fileinfo(edt->filepath);
  if (!edt->filepath.isEmpty()) {
    fname = fileinfo.fileName().replace("&", "&&");
    fpath = fileinfo.filePath();
  } else {
#ifdef ENABLE_PYTHON
    const bool isPython = (edt->language == LANG_PYTHON);
#else
    const bool isPython = false;
#endif
    fname = isPython ? "Untitled.py" : "Untitled.scad";
    fpath = fname;
  }
  return {fname, fpath};
}

std::tuple<QString, QString> TabManager::getEditorTabNameWithModifier(EditorInterface *edt)
{
  auto [fname, fpath] = getEditorTabName(edt);

  // Add the "modification" star if it was changed.
  bool isDirty = edt->isContentModified() || edt->parameterWidget->isModified();

  if (isDirty) fname += "*";

  return {fname, fpath};
}

void TabManager::setEditorTabName(const QString& tabName, const QString& tabToolTip,
                                  EditorInterface *edt)
{
  int index = tabWidget->indexOf(edt);
  tabWidget->setTabText(index, QString(tabName).replace("&", "&&"));
  tabWidget->setTabToolTip(index, tabToolTip);
}

void TabManager::updateTabIcon(EditorInterface *edt)
{
  if (!edt) return;

  int index = tabWidget->indexOf(edt);
  if (index < 0) return;

  QIcon icon;
  switch (edt->language) {
  case LANG_PYTHON: icon = QIcon(":/icons/filetype-python.svg"); break;
  case LANG_SCAD:
  default:          icon = QIcon(":/icons/filetype-openscad.svg"); break;
  }

  tabWidget->setTabIcon(index, icon);
}

bool TabManager::refreshDocument()
{
  bool file_opened = false;
  if (!editor->filepath.isEmpty()) {
    QFile file(editor->filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      if (!boost::algorithm::ends_with(editor->filepath, "Untitled.py")) {
        LOG("Failed to open file %1$s: %2$s", editor->filepath.toLocal8Bit().constData(),
            file.errorString().toLocal8Bit().constData());
      }
    } else {
      QTextStream reader(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      reader.setCodec("UTF-8");
#endif
      auto text = reader.readAll();
      LOG("Loaded design '%1$s'.", editor->filepath.toStdString());
      if (editor->toPlainText() != text) {
        editor->setPlainText(text);
        setContentRenderState();  // since last render
        editor->recomputeLanguageActive();
      }
      file_opened = true;
    }
  }
  if (file_opened) {
    parent->fileChangedOnDisk();
  }
  return file_opened;
}

QString TabManager::plainEditorTitleForMessages(EditorInterface *edt) const
{
  if (!edt->filepath.isEmpty()) {
    return QFileInfo(edt->filepath).fileName();
  }
#ifdef ENABLE_PYTHON
  return (edt->language == LANG_PYTHON) ? QStringLiteral("Untitled.py")
                                        : QStringLiteral("Untitled.scad");
#else
  return QStringLiteral("Untitled.scad");
#endif
}

bool TabManager::maybeSave(int x)
{
  auto *edt = (EditorInterface *)tabWidget->widget(x);
  if (edt->isContentModified() || edt->parameterWidget->isModified()) {
    const QString fname = plainEditorTitleForMessages(edt);
    QMessageBox box(parent);
    box.setText(QString(_("Do you want to save the changes you made to %1?")).arg(fname));
    box.setInformativeText(_("Your changes will be lost if you don't save them."));
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    box.setIcon(QMessageBox::Warning);
    box.setWindowModality(Qt::ApplicationModal);
    box.button(QMessageBox::Discard)->setText(_("Don't save"));
#ifdef Q_OS_MACOS
    // Cmd-D is the standard shortcut for this button on Mac
    box.button(QMessageBox::Discard)->setShortcut(QKeySequence("Ctrl+D"));
    box.button(QMessageBox::Discard)->setShortcutEnabled(true);
#endif
    auto ret = (QMessageBox::StandardButton)box.exec();

    if (ret == QMessageBox::Save) {
      return save(edt);
    } else if (ret == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

/*!
 * Called for whole window close, returning false will abort the close
 * operation.
 */
bool TabManager::shouldClose()
{
  bool hasUnsavedChanges = false;
  foreach (EditorInterface *edt, editorList) {
    if (edt->isContentModified() || edt->parameterWidget->isModified()) {
      hasUnsavedChanges = true;
      break;
    }
  }

  if (!hasUnsavedChanges) {
    return true;
  }

  UnsavedChangesDialog dialog(this, parent, parent);
  dialog.exec();

  switch (dialog.unsavedResult()) {
  case UnsavedChangesDialog::AllSaved:   return true;
  case UnsavedChangesDialog::DiscardAll: return true;
  case UnsavedChangesDialog::Cancel:
  default:                               return false;
  }
}

QString TabManager::getSessionFilePath()
{
  QSettings s;
  const QString configFile = s.fileName();
  if (configFile.isEmpty()) {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QStringLiteral("/session.json");
  }
  return QFileInfo(configFile).absolutePath() + QStringLiteral("/session.json");
}

QString TabManager::getAutosaveFilePath()
{
  const QString sessionPath = getSessionFilePath();
  return QFileInfo(sessionPath).absolutePath() + QStringLiteral("/session.autosave.json");
}

bool TabManager::hasDirtyTabs()
{
  for (auto *mainWin : scadApp->windowManager.getWindows()) {
    auto *tm = mainWin->tabManager;
    for (auto *edt : tm->editorList) {
      if (edt->isContentModified() || edt->parameterWidget->isModified()) return true;
    }
  }
  return false;
}

void TabManager::bumpSessionDirtyGeneration()
{
  ++sessionDirtyGenerationValue;
}

uint64_t TabManager::sessionDirtyGeneration()
{
  return sessionDirtyGenerationValue;
}

void TabManager::setSkipSessionSave(bool skip)
{
  skipSessionSaveOnQuit = skip;
}

bool TabManager::shouldSkipSessionSave()
{
  return skipSessionSaveOnQuit;
}

void TabManager::setTabSessionData(EditorInterface *edt, const QString& filepath, const QString& content,
                                   bool contentModified, bool parameterModified,
                                   const QByteArray& customizerState, std::optional<int> sessionLanguage)
{
  const QSignalBlocker blockEditor(edt);
  const QSignalBlocker blockParameters(edt->parameterWidget);
  edt->filepath = filepath;
  edt->setPlainText(content);
  edt->setContentModified(contentModified);
  edt->parameterWidget->setModified(parameterModified);
  if (!customizerState.isEmpty()) {
    edt->parameterWidget->setSessionState(customizerState);
  }
#ifdef ENABLE_PYTHON
  if (sessionLanguage.has_value()) {
    const int lang = *sessionLanguage;
    if (lang == LANG_PYTHON || lang == LANG_SCAD) {
      edt->setLanguageManually(lang);
    } else {
      edt->resetLanguageDetection();
      edt->recomputeLanguageActive();
    }
  } else {
    edt->resetLanguageDetection();
    edt->recomputeLanguageActive();
    // Old session files had no language field; untitled Python tabs use an empty path.
    if (filepath.isEmpty() && edt->language == LANG_SCAD) {
      const QString trimmed = content.trimmed();
      if (trimmed.startsWith(QStringLiteral("from openscad import"))) {
        edt->setLanguageManually(LANG_PYTHON);
      }
    }
  }
#else
  (void)sessionLanguage;
  edt->resetLanguageDetection();
  edt->recomputeLanguageActive();
#endif
  parent->onLanguageActiveChanged(edt->language);
  auto [fname, fpath] = getEditorTabNameWithModifier(edt);
  setEditorTabName(fname, fpath, edt);
  updateTabIcon(edt);
}

void TabManager::saveSession(const QString& path)
{
  QJsonArray tabs;
  for (int i = 0; i < tabWidget->count(); ++i) {
    auto *edt = static_cast<EditorInterface *>(tabWidget->widget(i));
    QJsonObject obj;
    obj.insert(QStringLiteral("filepath"), edt->filepath);
    obj.insert(QStringLiteral("content"), edt->toPlainText());
    obj.insert(QStringLiteral("contentModified"), edt->isContentModified());
    obj.insert(QStringLiteral("parameterModified"), edt->parameterWidget->isModified());
    obj.insert(QStringLiteral("language"), edt->language);
    int cursorLine = 0;
    int cursorColumn = 0;
    edt->getCursorPosition(&cursorLine, &cursorColumn);
    obj.insert(QStringLiteral("cursorLine"), cursorLine);
    obj.insert(QStringLiteral("cursorColumn"), cursorColumn);
    obj.insert(QStringLiteral("firstVisibleLine"), edt->firstVisibleLine());
    obj.insert(QStringLiteral("findState"), edt->findState);
    const QByteArray customizerState = edt->parameterWidget->getSessionState();
    if (!customizerState.isEmpty()) {
      obj.insert(QStringLiteral("customizerState"), QString::fromUtf8(customizerState));
    }
    if (!edt->filepath.isEmpty()) {
      obj.insert(QStringLiteral("diskIdentity"),
                 QString::fromStdString(MainWindow::autoReloadIdentityForPath(edt->filepath)));
    }
    tabs.append(obj);
  }
  QJsonObject win;
  win.insert(QStringLiteral("tabs"), tabs);
  win.insert(QStringLiteral("currentIndex"), tabWidget->currentIndex());
  if (parent && parent->qglview) {
    QJsonObject view;
    view.insert(QStringLiteral("vpt"), vec3ToJson(parent->qglview->cam.getVpt()));
    view.insert(QStringLiteral("vpr"), vec3ToJson(parent->qglview->cam.getVpr()));
    view.insert(QStringLiteral("vpd"), parent->qglview->cam.zoomValue());
    view.insert(QStringLiteral("vpf"), parent->qglview->cam.fovValue());
    view.insert(QStringLiteral("projection"), parent->qglview->orthoMode()
                                                ? QStringLiteral("orthogonal")
                                                : QStringLiteral("perspective"));
    win.insert(QStringLiteral("viewState"), view);
  }
  if (parent) {
    QJsonObject findPanel;
    findPanel.insert(QStringLiteral("text"), parent->findInputField->text());
    findPanel.insert(QStringLiteral("replaceText"), parent->replaceInputField->text());
    win.insert(QStringLiteral("findPanel"), findPanel);
  }

  QJsonArray windows;
  windows.append(win);

  QJsonObject root;
  root.insert(QStringLiteral("version"), SESSION_VERSION);
  root.insert(QStringLiteral("windows"), windows);
  QString error;
  if (!writeSessionFile(root, path, &error)) {
    warnSessionSaveFailure(path, error);
  }
}

/*!
 * Save session state from ALL open windows into a single session file.
 * Called by the "Quit" action so every window is persisted.
 */
bool TabManager::saveGlobalSession(const QString& path, QString *error, bool showWarning)
{
  QJsonArray windows;
  std::vector<MainWindow *> windowOrder;
  windowOrder.reserve(static_cast<size_t>(scadApp->windowManager.getWindows().size()));
  for (auto *mainWin : scadApp->windowManager.getWindows()) {
    windowOrder.push_back(mainWin);
  }
  std::sort(windowOrder.begin(), windowOrder.end(), [](const MainWindow *a, const MainWindow *b) {
    return reinterpret_cast<std::uintptr_t>(a) < reinterpret_cast<std::uintptr_t>(b);
  });

  MainWindow *const lastActive = scadApp->windowManager.getLastActive();
  int activeWindowIndex = 0;
  int windowSerial = 0;
  for (MainWindow *mainWin : windowOrder) {
    if (mainWin == lastActive) {
      activeWindowIndex = windowSerial;
    }
    ++windowSerial;
    auto *tm = mainWin->tabManager;
    QJsonArray tabs;
    for (int i = 0; i < tm->tabWidget->count(); ++i) {
      auto *edt = static_cast<EditorInterface *>(tm->tabWidget->widget(i));
      QJsonObject obj;
      obj.insert(QStringLiteral("filepath"), edt->filepath);
      obj.insert(QStringLiteral("content"), edt->toPlainText());
      obj.insert(QStringLiteral("contentModified"), edt->isContentModified());
      obj.insert(QStringLiteral("parameterModified"), edt->parameterWidget->isModified());
      obj.insert(QStringLiteral("language"), edt->language);
      int cursorLine = 0;
      int cursorColumn = 0;
      edt->getCursorPosition(&cursorLine, &cursorColumn);
      obj.insert(QStringLiteral("cursorLine"), cursorLine);
      obj.insert(QStringLiteral("cursorColumn"), cursorColumn);
      obj.insert(QStringLiteral("firstVisibleLine"), edt->firstVisibleLine());
      obj.insert(QStringLiteral("findState"), edt->findState);
      const QByteArray customizerState = edt->parameterWidget->getSessionState();
      if (!customizerState.isEmpty()) {
        obj.insert(QStringLiteral("customizerState"), QString::fromUtf8(customizerState));
      }
      if (!edt->filepath.isEmpty()) {
        obj.insert(QStringLiteral("diskIdentity"),
                   QString::fromStdString(MainWindow::autoReloadIdentityForPath(edt->filepath)));
      }
      tabs.append(obj);
    }
    QJsonObject win;
    win.insert(QStringLiteral("tabs"), tabs);
    win.insert(QStringLiteral("currentIndex"), tm->tabWidget->currentIndex());
    if (mainWin && mainWin->qglview) {
      QJsonObject view;
      view.insert(QStringLiteral("vpt"), vec3ToJson(mainWin->qglview->cam.getVpt()));
      view.insert(QStringLiteral("vpr"), vec3ToJson(mainWin->qglview->cam.getVpr()));
      view.insert(QStringLiteral("vpd"), mainWin->qglview->cam.zoomValue());
      view.insert(QStringLiteral("vpf"), mainWin->qglview->cam.fovValue());
      view.insert(QStringLiteral("projection"), mainWin->qglview->orthoMode()
                                                  ? QStringLiteral("orthogonal")
                                                  : QStringLiteral("perspective"));
      win.insert(QStringLiteral("viewState"), view);
    }
    if (mainWin) {
      QJsonObject findPanel;
      findPanel.insert(QStringLiteral("text"), mainWin->findInputField->text());
      findPanel.insert(QStringLiteral("replaceText"), mainWin->replaceInputField->text());
      win.insert(QStringLiteral("findPanel"), findPanel);
    }
    windows.append(win);
  }

  QJsonObject root;
  root.insert(QStringLiteral("version"), SESSION_VERSION);
  root.insert(QStringLiteral("windows"), windows);
  root.insert(QStringLiteral("activeWindowIndex"), activeWindowIndex);

  QString localError;
  QString *targetError = error ? error : &localError;
  const bool ok = writeSessionFile(root, path, targetError);
  if (!ok && showWarning) {
    warnSessionSaveFailure(path, *targetError);
  }
  return ok;
}

/*!
 * Migrate a session JSON object from \a fromVersion to SESSION_VERSION.
 * Add a case for each version transition when the schema changes.
 * Returns true on success, false if migration is not possible.
 */
bool TabManager::migrateSession(QJsonObject& root, int fromVersion)
{
  // Apply migrations sequentially: v1->v2, v2->v3, etc.
  for (int v = fromVersion; v < SESSION_VERSION; ++v) {
    switch (v) {
    case 1: {
      // Migrate v1 -> v2: wrap flat tabs/currentIndex into a "windows" array.
      QJsonArray windows;
      QJsonObject win;
      win.insert(QStringLiteral("tabs"), root.value(QStringLiteral("tabs")));
      win.insert(QStringLiteral("currentIndex"), root.value(QStringLiteral("currentIndex")));
      windows.append(win);
      root.remove(QStringLiteral("tabs"));
      root.remove(QStringLiteral("currentIndex"));
      root.insert(QStringLiteral("windows"), windows);
      break;
    }
    case 2: {
      // v2 -> v3: optional root-level activeWindowIndex (default 0 when absent).
      break;
    }
    case 3: {
      // v3 -> v4: optional per-tab diskIdentity (mtime+size at session save).
      break;
    }
    default:
      // No known migration path from this version
      return false;
    }
  }
  root.insert(QStringLiteral("version"), SESSION_VERSION);
  return true;
}

TabManager::SessionFileReadStatus TabManager::readSessionFileRoot(const QString& path,
                                                                  QJsonObject *outRoot,
                                                                  QString *openError, int *tooNewVersion,
                                                                  int *migrateFailedAtVersion)
{
  if (!outRoot) {
    return SessionFileReadStatus::InvalidJson;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (openError) {
      *openError = file.errorString();
    }
    return SessionFileReadStatus::OpenFailed;
  }
  const QByteArray rawData = file.readAll();
  file.close();

  const QJsonDocument doc = QJsonDocument::fromJson(rawData);
  if (!doc.isObject()) {
    return SessionFileReadStatus::InvalidJson;
  }

  QJsonObject root = doc.object();
  const int fileVersion = root.value(QStringLiteral("version")).toInt(1);

  if (fileVersion > SESSION_VERSION) {
    if (tooNewVersion) {
      *tooNewVersion = fileVersion;
    }
    return SessionFileReadStatus::TooNew;
  }

  if (fileVersion < SESSION_VERSION) {
    if (!migrateSession(root, fileVersion)) {
      if (migrateFailedAtVersion) {
        *migrateFailedAtVersion = fileVersion;
      }
      return SessionFileReadStatus::MigrateFailed;
    }
  }

  *outRoot = std::move(root);
  return SessionFileReadStatus::Ok;
}

bool TabManager::restoreSession(const QString& path, int windowIndex)
{
  QJsonObject root;
  QString openErr;
  int tooNewVer = 0;
  int migrateFailVer = 0;
  const auto readSt = readSessionFileRoot(path, &root, &openErr, &tooNewVer, &migrateFailVer);
  switch (readSt) {
  case SessionFileReadStatus::OpenFailed:
    QMessageBox::warning(parent, QString(_("Session Restore")),
                         QString(_("Could not open the session file for reading:\n%1\n\n%2\n\n"
                                   "Starting with a fresh session."))
                           .arg(path)
                           .arg(openErr));
    return false;
  case SessionFileReadStatus::InvalidJson:
    QMessageBox::warning(parent, QString(_("Session Restore")),
                         QString(_("The session file is corrupt or unreadable:\n%1\n\n"
                                   "The file has not been deleted — you may inspect it manually.\n"
                                   "Starting with a fresh session."))
                           .arg(path));
    return false;
  case SessionFileReadStatus::TooNew:
    QMessageBox::critical(
      parent, QString(_("Session Restore")),
      QString(_("The session file was created by a newer version of PythonSCAD "
                "(session version %1, but this build only supports up to version %2).\n\n"
                "Please upgrade PythonSCAD or delete the session file:\n%3"))
        .arg(tooNewVer)
        .arg(SESSION_VERSION)
        .arg(path));
    return false;
  case SessionFileReadStatus::MigrateFailed:
    QMessageBox::warning(
      parent, QString(_("Session Restore")),
      QString(_("The session file uses an old format (version %1) that cannot be "
                "migrated to the current format (version %2). Starting with a fresh session."))
        .arg(migrateFailVer)
        .arg(SESSION_VERSION));
    return false;
  case SessionFileReadStatus::Ok: break;
  }

  const QJsonArray windows = root.value(QStringLiteral("windows")).toArray();
  if (windowIndex < 0 || windowIndex >= windows.size()) return false;

  const QJsonObject win = windows[windowIndex].toObject();
  const QJsonObject viewState = win.value(QStringLiteral("viewState")).toObject();
  const QJsonObject findPanel = win.value(QStringLiteral("findPanel")).toObject();
  const QJsonArray tabs = win.value(QStringLiteral("tabs")).toArray();
  if (tabs.isEmpty()) return false;
  const int savedCurrentIndex = win.value(QStringLiteral("currentIndex")).toInt(0);
  int firstMissingIndex = -1;
  int missingCount = 0;

  // Block signals during restore to prevent tab-switch signals from triggering
  // compile/preview (which calls initPython) before the constructor is finished.
  const bool oldBlocked = tabWidget->blockSignals(true);

  QVector<QString> diskIdentityAtSave;
  diskIdentityAtSave.reserve(tabs.size());

  for (int i = 0; i < tabs.size(); ++i) {
    const QJsonObject obj = tabs[i].toObject();
    const QString filepath = obj.value(QStringLiteral("filepath")).toString();
    QString content = obj.value(QStringLiteral("content")).toString();
    const bool contentModified = obj.value(QStringLiteral("contentModified")).toBool();
    const bool parameterModified = obj.value(QStringLiteral("parameterModified")).toBool();
    const int cursorLine = obj.value(QStringLiteral("cursorLine")).toInt(-1);
    const int cursorColumn = obj.value(QStringLiteral("cursorColumn")).toInt(-1);
    const int firstVisibleLine = obj.value(QStringLiteral("firstVisibleLine")).toInt(-1);
    int findState = obj.value(QStringLiteral("findState")).toInt(TabManager::FIND_HIDDEN);
    const QByteArray customizerState = obj.value(QStringLiteral("customizerState")).toString().toUtf8();
    std::optional<int> sessionLanguage;
    if (obj.contains(QStringLiteral("language"))) {
      sessionLanguage = obj.value(QStringLiteral("language")).toInt();
    }

    const QFileInfo fileInfo(filepath);
    if (!filepath.isEmpty() && fileInfo.isAbsolute() && !fileInfo.exists()) {
      if (firstMissingIndex < 0) {
        firstMissingIndex = i;
      }
      ++missingCount;
    }

    // If tab had no unsaved changes, reload from disk (match behavior when app is running).
    if (!filepath.isEmpty() && !contentModified) {
      QFile diskFile(filepath);
      if (diskFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&diskFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        in.setEncoding(QStringConverter::Utf8);
#else
        in.setCodec("UTF-8");
#endif
        content = in.readAll();
        diskFile.close();
      }
    }

    EditorInterface *edt;
    if (i == 0) {
      edt = static_cast<EditorInterface *>(tabWidget->widget(0));
    } else {
      createTab(QString(), false);
      edt = editor;
    }
    setTabSessionData(edt, filepath, content, contentModified, parameterModified, customizerState,
                      sessionLanguage);
    if (findState < TabManager::FIND_HIDDEN || findState > TabManager::FIND_REPLACE_VISIBLE) {
      findState = TabManager::FIND_HIDDEN;
    }
    edt->findState = findState;
    if (cursorLine >= 0 && cursorColumn >= 0) {
      edt->setCursorPosition(cursorLine, cursorColumn);
    }
    if (firstVisibleLine >= 0) {
      edt->setFirstVisibleLine(firstVisibleLine);
    }

    diskIdentityAtSave.append(obj.value(QStringLiteral("diskIdentity")).toString());
  }
  // Prime autoReloadId so auto-reload does not treat an empty id as "file changed" and reload
  // over an unsaved buffer. For dirty tabs, if the on-disk file changed since the session was
  // saved (diskIdentity in JSON != current stat), keep the saved fingerprint so the next
  // fileChangedOnDisk() still reports a change and the usual reload dialog appears.
  for (int ti = 0; ti < tabWidget->count(); ++ti) {
    auto *syncEdt = static_cast<EditorInterface *>(tabWidget->widget(ti));
    if (syncEdt->filepath.isEmpty()) {
      continue;
    }
    const std::string currentId = MainWindow::autoReloadIdentityForPath(syncEdt->filepath);
    if (currentId.empty()) {
      continue;
    }
    if (!syncEdt->isContentModified()) {
      syncEdt->autoReloadId = currentId;
      continue;
    }
    const QString savedId = ti < diskIdentityAtSave.size() ? diskIdentityAtSave[ti] : QString();
    if (!savedId.isEmpty()) {
      const std::string savedStd = savedId.toStdString();
      if (savedStd != currentId) {
        syncEdt->autoReloadId = savedStd;
        continue;
      }
    }
    syncEdt->autoReloadId = currentId;
  }
  const int currentIndex = std::max(0, std::min(savedCurrentIndex, tabWidget->count() - 1));
  tabWidget->setCurrentIndex(currentIndex);

  tabWidget->blockSignals(oldBlocked);

  // Manually trigger the tab-switched handler now that construction is far enough along.
  tabSwitched(currentIndex);

  if (!findPanel.isEmpty()) {
    const QSignalBlocker findBlocker(parent->findInputField);
    const QSignalBlocker replaceBlocker(parent->replaceInputField);
    parent->findInputField->setText(findPanel.value(QStringLiteral("text")).toString());
    parent->replaceInputField->setText(findPanel.value(QStringLiteral("replaceText")).toString());
  }

  updateFindState();

  if (!viewState.isEmpty() && parent && parent->qglview) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (jsonToVec3(viewState.value(QStringLiteral("vpt")), &x, &y, &z)) {
      parent->qglview->cam.setVpt(x, y, z);
    }
    if (jsonToVec3(viewState.value(QStringLiteral("vpr")), &x, &y, &z)) {
      parent->qglview->cam.setVpr(x, y, z);
    }
    const double vpd = viewState.value(QStringLiteral("vpd")).toDouble(-1.0);
    if (vpd > 0.0) parent->qglview->cam.setVpd(vpd);
    const double vpf = viewState.value(QStringLiteral("vpf")).toDouble(-1.0);
    if (vpf > 0.0) parent->qglview->cam.setVpf(vpf);
    const QString projection = viewState.value(QStringLiteral("projection")).toString();
    const bool isOrthogonal = projection == QStringLiteral("orthogonal");
    parent->qglview->setOrthoMode(isOrthogonal);
    parent->viewActionOrthogonal->setChecked(isOrthogonal);
    parent->viewActionPerspective->setChecked(!isOrthogonal);
    parent->qglview->update();
    parent->viewportControlWidget->cameraChanged();
  }

  if (missingCount > 0) {
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QString(_("Session Restore")));
    box.setText(QString(_("%1 file(s) could not be found on disk.")).arg(missingCount));
    box.setInformativeText(
      QString(_("The session contents were restored, but the file paths are stale.\n"
                "Use Save As to pick a new location.")));
    auto *saveAsButton = box.addButton(QString(_("Save As...")), QMessageBox::AcceptRole);
    box.addButton(QString(_("Dismiss")), QMessageBox::RejectRole);
    box.setDefaultButton(saveAsButton);
    box.exec();
    if (box.clickedButton() == saveAsButton && firstMissingIndex >= 0) {
      tabWidget->setCurrentIndex(firstMissingIndex);
      QMetaObject::invokeMethod(parent, "on_fileActionSaveAs_triggered", Qt::QueuedConnection);
    }
  }

  parent->setWindowTitle(tabWidget->tabText(tabWidget->currentIndex()).replace("&&", "&"));
  parent->updateRecentFileActions();
  return true;
}

/*!
 * Returns the number of windows stored in the session file, or 0 on error.
 * Performs lightweight parsing + optional migration without creating any GUI.
 */
int TabManager::sessionWindowCount(const QString& path)
{
  QJsonObject root;
  if (readSessionFileRoot(path, &root) != SessionFileReadStatus::Ok) return 0;
  return root.value(QStringLiteral("windows")).toArray().size();
}

int TabManager::sessionActiveWindowIndex(const QString& path)
{
  QJsonObject root;
  if (readSessionFileRoot(path, &root) != SessionFileReadStatus::Ok) return 0;

  const QJsonArray windowArray = root.value(QStringLiteral("windows")).toArray();
  const int winCount = windowArray.size();
  if (winCount <= 0) return 0;

  int idx = root.value(QStringLiteral("activeWindowIndex")).toInt(0);
  idx = std::max(0, std::min(idx, winCount - 1));
  return idx;
}

bool TabManager::sessionHasOnlyEmptyTab(const QString& path)
{
  QJsonObject root;
  if (readSessionFileRoot(path, &root) != SessionFileReadStatus::Ok) return false;

  const QJsonArray windows = root.value(QStringLiteral("windows")).toArray();
  if (windows.size() != 1) return false;

  const QJsonObject win = windows[0].toObject();
  const QJsonArray tabs = win.value(QStringLiteral("tabs")).toArray();
  if (tabs.size() != 1) return false;

  const QJsonObject tab = tabs[0].toObject();
  const QString filepath = tab.value(QStringLiteral("filepath")).toString();
  const bool contentModified = tab.value(QStringLiteral("contentModified")).toBool();

  // Trivial session: one window, one tab, nothing saved to a path, editor not dirty.
  // Content length and customizer state are ignored — template/placeholder text and
  // preset UI alone should not force auto-restore on startup.
  return filepath.isEmpty() && !contentModified;
}

/*!
 * Remove the session file so the next launch starts with a fresh session.
 * Called when the user explicitly closes all windows (as opposed to Quit).
 */
void TabManager::removeSessionFile()
{
  const QString path = getSessionFilePath();
  QFile::remove(path);
}

void TabManager::saveError(const QIODevice& file, const std::string& msg, const QString& filepath)
{
  const std::string fileName = filepath.toStdString();
  LOG("%1$s %2$s (%3$s)", msg.c_str(), fileName, file.errorString().toStdString());

  const std::string dialogFormatStr = msg + "\n\"%1\"\n(%2)";
  const QString dialogFormat(dialogFormatStr.c_str());
  QMessageBox::warning(parent, parent->windowTitle(),
                       dialogFormat.arg(filepath).arg(file.errorString()));
}

/*!
 * Save current document.
 * Should _always_ write to disk, since this is called by SaveAs - i.e. don't
 * try to be smart and check for document modification here.
 */
bool TabManager::save(EditorInterface *edt)
{
  assert(edt != nullptr);
  if (edt->filepath.endsWith("Untitled.py")) edt->filepath = "";
  if (edt->filepath.isEmpty()) {
    return saveAs(edt);
  } else {
    return save(edt, edt->filepath);
  }
}

bool TabManager::save(EditorInterface *edt, const QString& path)
{
  // If available (>= Qt 5.1), use QSaveFile to ensure the file is not
  // destroyed if the device is full. Unfortunately this is not working
  // as advertised (at least in Qt 5.3) as it does not detect the device
  // full properly and happily commits a 0 byte file.
  // Checking the QTextStream status flag after flush() seems to catch
  // this condition.
  // FIXME jeff hayes - i have recently seen a better way to handle this, when i have found that note
  // again i will revist this
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    saveError(file, _("Failed to open file for writing"), path);
    return false;
  }

  QTextStream writer(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  writer.setCodec("UTF-8");
#endif
  writer << edt->toPlainText();
  writer.flush();
  bool saveOk = writer.status() == QTextStream::Ok;
  if (saveOk) {
    saveOk = file.commit();
  } else {
    file.cancelWriting();
  }
  if (saveOk) {
    LOG("Saved design '%1$s'.", path.toStdString());
    edt->parameterWidget->saveFile(path);
    edt->setContentModified(false);
    edt->parameterWidget->setModified(false);
    parent->updateRecentFiles(path);
    edt->filepath = path;
  } else {
    saveError(file, _("Error saving design"), path);
  }
  return saveOk;
}

bool TabManager::saveAs(EditorInterface *edt)
{
  assert(edt != nullptr);

  const auto defaultName = (edt->language == LANG_PYTHON) ? _("Untitled.py") : _("Untitled.scad");
  const auto dir = edt->filepath.isEmpty() ? defaultName : edt->filepath;
#ifdef ENABLE_PYTHON
  QString selectedFilter;
  QString pythonFilter = _("PythonSCAD Designs (*.py)");
  auto filename = QFileDialog::getSaveFileName(parent, _("Save File"), dir,
                                               QString("%1").arg(pythonFilter), &selectedFilter);
#else
  auto filename =
    QFileDialog::getSaveFileName(parent, _("Save File"), dir, _("OpenSCAD Designs (*.scad)"));
#endif
  if (filename.isEmpty()) {
    return false;
  }

  auto guard = parent->scopedSetCurrentOutput();

  if (QFileInfo(filename).suffix().isEmpty()) {
#ifdef ENABLE_PYTHON
    // Check if the user selected the Python filter
    if (selectedFilter == pythonFilter) {
      filename.append(".py");
    } else {
      // For other cases, use .scad as the default extension
      filename.append(".scad");
    }
#else
    filename.append(".scad");
#endif

    // Manual overwrite check since Qt doesn't do it, when using the
    // defaultSuffix property
    const QFileInfo info(filename);
    if (info.exists()) {
      const auto text =
        QString(_("%1 already exists.\nDo you want to replace it?")).arg(info.fileName());
      if (QMessageBox::warning(parent, parent->windowTitle(), text, QMessageBox::Yes | QMessageBox::No,
                               QMessageBox::No) != QMessageBox::Yes) {
        return false;
      }
    }
  }

  return saveAs(edt, filename);
}

bool TabManager::saveAs(EditorInterface *edt, const QString& filepath)
{
  bool saveOk = save(edt, filepath);
  if (saveOk) {
    edt->resetLanguageDetection();
    parent->onLanguageActiveChanged(edt->language);
    updateTabIcon(edt);
    auto [fname, fpath] = getEditorTabNameWithModifier(edt);
    setEditorTabName(fname, fpath, edt);
    parent->setWindowTitle(fname);
  }
  return saveOk;
}

/*
 If the editor content has not yet been saved it will be saved
 to Untitled.scad in the application root directory.
 Otherwise append  "_copy" to the base file name.
 The name of the editor tab should NOT be changed
 */
bool TabManager::saveACopy(EditorInterface *edt)
{
  assert(edt != nullptr);

  const QString path = edt->filepath;

  QString suffix = ".scad";
  QDir dir(_("Untitled.scad"));
  if (path.endsWith(".py")) {
    suffix = ".py";
    dir = QDir(_("Untitled.py"));
  }

  if (!path.isEmpty()) {
    QFileInfo info(path);
    QString filecopy(info.absolutePath() % "/" % info.baseName() % "_copy" % suffix);
    dir.setPath(filecopy);
  }

  QFileDialog saveCopyDialog;
  saveCopyDialog.setAcceptMode(QFileDialog::AcceptSave);  // Set the dialog to "Save" mode.
  saveCopyDialog.setWindowTitle("Save A Copy");

  saveCopyDialog.setNameFilter("PythonSCAD Designs (*.py, *.scad)");

  saveCopyDialog.setDefaultSuffix("scad");
  saveCopyDialog.setViewMode(QFileDialog::List);
  saveCopyDialog.setDirectory(dir);

  if (saveCopyDialog.exec() != QDialog::Accepted) return false;

  auto guard = parent->scopedSetCurrentOutput();

  QStringList selectedFiles = saveCopyDialog.selectedFiles();
  if (selectedFiles.isEmpty()) return false;

  QString savefile = selectedFiles.first();

  if (savefile.isEmpty()) return false;

  QSaveFile file(savefile);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    saveError(file, _("Failed to open file for writing"), path);
    return false;
  }
  QTextStream writer(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  writer.setCodec("UTF-8");
#endif
  writer << edt->toPlainText();
  writer.flush();

  bool saveOk = writer.status() == QTextStream::Ok;
  if (!saveOk) file.cancelWriting();
  else {
    saveOk = file.commit();
    if (!saveOk) saveError(file, _("Error saving design"), savefile);
    else LOG("Saved design '%1$s'.", savefile.toLocal8Bit().constData());
  }
  return saveOk;
}

bool TabManager::saveAll()
{
  foreach (EditorInterface *edt, editorList) {
    if (edt->isContentModified() || edt->parameterWidget->isModified()) {
      if (!save(edt)) {
        return false;
      }
    }
  }
  return true;
}

void TabManager::zoomIn()
{
  if (editor) {
    editor->zoomIn();
  }
}

void TabManager::zoomOut()
{
  if (editor) {
    editor->zoomOut();
  }
}
