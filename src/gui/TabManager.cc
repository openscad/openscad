#include "gui/TabManager.h"

#include <QApplication>
#include <QPoint>
#include <QTabBar>
#include <QWidget>
#include <cassert>
#include <functional>
#include <exception>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QShortcut>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QDesktopServices>
#include <Qsci/qscicommand.h>
#include <Qsci/qscicommandset.h>

#include "gui/Editor.h"
#include "gui/ImportUtils.h"
#include "gui/ScintillaEditor.h"
#include "gui/Preferences.h"
#include "gui/MainWindow.h"

#include <cstddef>

TabManager::TabManager(MainWindow *o)
{
  par = o;

  tabWidget = new QTabWidget();
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);
  tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(tabWidget, &QTabWidget::tabCloseRequested, this, &TabManager::closeTabRequested);
  connect(tabWidget, &QTabWidget::customContextMenuRequested, this, &TabManager::showTabHeaderContextMenu);

  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::stopAnimation);
  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::updateFindState);
  connect(tabWidget, &QTabWidget::currentChanged, this, &TabManager::tabSwitched);
}

QTabBar::ButtonPosition TabManager::getClosingButtonPosition()
{
  auto bar = tabWidget->tabBar();
  return (QTabBar::ButtonPosition)bar->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, bar);
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

  auto currentEditor = (EditorInterface *)tabWidget->widget(x);
  auto numberOfOpenTabs = tabWidget->count();
  // Hides all the closing button except the one on the currently focused editor
  for (int idx = 0; idx < numberOfOpenTabs; ++idx) {
    bool isVisible = idx == x && numberOfOpenTabs > 1;
    setTabsCloseButtonVisibility(idx, isVisible);
  }

  emit currentEditorChanged(currentEditor);
}

void TabManager::closeTabRequested(int x)
{
  assert(tabWidget != nullptr);
  if (!maybeSave(x)) return;

  auto *closingEditor = qobject_cast<EditorInterface *>(tabWidget->widget(x));
  assert(closingEditor != nullptr);

  emit editorAboutToClose(closingEditor);

  tabWidget->removeTab(x);
  emit tabCountChanged(tabWidget->count());

  delete closingEditor->parameterWidget;
  delete closingEditor;
}

void TabManager::closeCurrentTab()
{
  assert(tabWidget != nullptr);

  /* Close tab or close the current window if only one tab is open. */
  if (tabWidget->count() > 1) this->closeTabRequested(tabWidget->currentIndex());
  else par->close();
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

void TabManager::actionNew()
{
  if (!par->editorDock->isVisible()) par->editorDock->setVisible(true);   //if editor hidden, make it visible
  createTab("");
}

QSet<EditorInterface *> TabManager::editors() const
{
  QSet<EditorInterface *> editors;
  for (int i = 0; i < tabWidget->count(); ++i)
    editors.insert(static_cast<EditorInterface *>(tabWidget->widget(i)));
  return editors;
}

void TabManager::open(const QString& filename)
{
  assert(!filename.isEmpty());
  for (auto edt: editors()) {
    if (filename == edt->filepath) {
      tabWidget->setCurrentWidget(edt);
      return;
    }
  }

  if (activeEditor()->filepath.isEmpty() && !activeEditor()->isContentModified() && !activeEditor()->parameterWidget->isModified()) {
    openTabFile(filename);
  } else {
    createTab(filename);
  }
}

void TabManager::createTab(const QString& filename)
{
  assert(par != nullptr);

  auto scintillaEditor = new ScintillaEditor(tabWidget);
  scintillaEditor->parameterWidget = new ParameterWidget(par->parameterDock);
  connect(scintillaEditor->parameterWidget, &ParameterWidget::parametersChanged, par, &MainWindow::actionRenderPreview);
  par->parameterDock->setWidget(scintillaEditor->parameterWidget);

  // clearing default mapping of keyboard shortcut for font size
  QsciCommandSet *qcmdset = scintillaEditor->qsci->standardCommands();
  QsciCommand *qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Plus);
  qcmd->setKey(0);
  qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Minus);
  qcmd->setKey(0);

  connect(scintillaEditor, &ScintillaEditor::uriDropped, par, &MainWindow::handleFileDrop);
  connect(scintillaEditor, &ScintillaEditor::previewRequest, par, &MainWindow::actionRenderPreview);
  connect(scintillaEditor, &EditorInterface::showContextMenuEvent, this, &TabManager::showContextMenuEvent);
  connect(scintillaEditor, &EditorInterface::focusIn, this, [ scintillaEditor, this ]() {
    par->setLastFocus(scintillaEditor);
  });

  connect(GlobalPreferences::inst(), &Preferences::editorConfigChanged, scintillaEditor, &ScintillaEditor::applySettings);
  connect(GlobalPreferences::inst(), &Preferences::autocompleteChanged, scintillaEditor, &ScintillaEditor::onAutocompleteChanged);
  connect(GlobalPreferences::inst(), &Preferences::characterThresholdChanged, scintillaEditor, &ScintillaEditor::onCharacterThresholdChanged);
  scintillaEditor->applySettings();
  scintillaEditor->addTemplate();

  connect(par->editActionZoomTextIn, &QAction::triggered, scintillaEditor, &EditorInterface::zoomIn);
  connect(par->editActionZoomTextOut, &QAction::triggered, scintillaEditor, &EditorInterface::zoomOut);

  connect(scintillaEditor, &EditorInterface::contentsChanged, this, &TabManager::updateActionUndoState);
  connect(scintillaEditor, &EditorInterface::contentsChanged, par,  &MainWindow::editorContentChanged);
  connect(scintillaEditor, &EditorInterface::contentsChanged, this, &TabManager::setContentRenderState);
  connect(scintillaEditor, &EditorInterface::modificationChanged, this, &TabManager::onTabModified);
  connect(scintillaEditor->parameterWidget, &ParameterWidget::modificationChanged, [scintillaEditor, this] {
    std::cout << "Parameter Widget modificaiton changed " << std::endl;
    onTabModified(scintillaEditor);
  });

  connect(GlobalPreferences::inst(), &Preferences::fontChanged, scintillaEditor, &EditorInterface::initFont);
  connect(GlobalPreferences::inst(), &Preferences::syntaxHighlightChanged, scintillaEditor, &EditorInterface::setHighlightScheme);
  scintillaEditor->initFont(GlobalPreferences::inst()->getValue("editor/fontfamily").toString(), GlobalPreferences::inst()->getValue("editor/fontsize").toUInt());
  scintillaEditor->setHighlightScheme(GlobalPreferences::inst()->getValue("editor/syntaxhighlight").toString());

  connect(scintillaEditor, &ScintillaEditor::hyperlinkIndicatorClicked, this, &TabManager::onHyperlinkIndicatorClicked);

  // Get the name of the tab in editor
  auto [fname, fpath] = getEditorTabNameWithModifier(scintillaEditor);
  int index = tabWidget->addTab(scintillaEditor, fname);

  // When a new tabbar is created, the close button is visible so we need to hide it.
  // The surprising thing is that this cannot be handled through the tabSwitched signal trigger at addTab
  // because there is no button yet created when the signal is fired.
  setTabsCloseButtonVisibility(index, false);

  if (tabWidget->currentWidget() != scintillaEditor) {
    tabWidget->setCurrentWidget(scintillaEditor);
  }

  // Fill the editor with the content of the file
  if (filename.isEmpty()) {
    scintillaEditor->filepath = "";
  } else {
    openTabFile(filename);
  }

  emit tabCountChanged(tabWidget->count());
}

size_t TabManager::count()
{
  return tabWidget->count();
}

void TabManager::highlightError(int i)
{
  activeEditor()->highlightError(i);
}

void TabManager::unhighlightLastError()
{
  activeEditor()->unhighlightLastError();
}

void TabManager::undo()
{
  activeEditor()->undo();
}

void TabManager::redo()
{
  activeEditor()->redo();
}

void TabManager::cut()
{
  activeEditor()->cut();
}

void TabManager::copy()
{
  activeEditor()->copy();
}

void TabManager::paste()
{
  activeEditor()->paste();
}

void TabManager::indentSelection()
{
  activeEditor()->indentSelection();
}

void TabManager::unindentSelection()
{
  activeEditor()->unindentSelection();
}

void TabManager::commentSelection()
{
  activeEditor()->commentSelection();
}

void TabManager::uncommentSelection()
{
  activeEditor()->uncommentSelection();
}

void TabManager::toggleBookmark()
{
  activeEditor()->toggleBookmark();
}

void TabManager::nextBookmark()
{
  activeEditor()->nextBookmark();
}

void TabManager::prevBookmark()
{
  activeEditor()->prevBookmark();
}

void TabManager::jumpToNextError()
{
  activeEditor()->jumpToNextError();
}

void TabManager::setFocus()
{
  activeEditor()->setFocus();
}

void TabManager::updateActionUndoState()
{
  par->editActionUndo->setEnabled(activeEditor()->canUndo());
}

void TabManager::onHyperlinkIndicatorClicked(int val)
{
  const QString filename = QString::fromStdString(activeEditor()->indicatorData[val].path);
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
  applyAction(QObject::sender(), [](int, EditorInterface *edt){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QFileInfo(edt->filepath).fileName());
  });
}

void TabManager::copyFilePath()
{
  applyAction(QObject::sender(), [](int, EditorInterface *edt){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(edt->filepath);
  });
}

void TabManager::openFolder()
{
  applyAction(QObject::sender(), [](int, EditorInterface *edt){
    auto dir = QFileInfo(edt->filepath).dir();
    if (dir.exists()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
    }
  });
}

void TabManager::closeTab()
{
  applyAction(QObject::sender(), [this](int idx, EditorInterface *){
    closeTabRequested(idx);
  });
}

void TabManager::showContextMenuEvent(const QPoint& pos)
{
  auto menu = activeEditor()->createStandardContextMenu();

  menu->addSeparator();
  menu->addAction(par->editActionFind);
  menu->addAction(par->editActionFindNext);
  menu->addAction(par->editActionFindPrevious);
  menu->addSeparator();
  menu->addAction(par->editActionInsertTemplate);
  menu->addAction(par->editActionFoldAll);
  menu->exec(activeEditor()->mapToGlobal(pos));

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

  // Don't allow to close the last tab.
  if (tabWidget->count() <= 1) closeAction->setDisabled(true);

  menu.addAction(copyFileNameAction);
  menu.addAction(copyFilePathAction);
  menu.addSeparator();
  menu.addAction(openFolderAction);
  menu.addSeparator();
  menu.addAction(closeAction);

  QPoint globalCursorPos = QCursor::pos();
  menu.exec(globalCursorPos);
}

EditorInterface *TabManager::activeEditor(){
  return static_cast<EditorInterface *>(tabWidget->currentWidget());
}

void TabManager::setContentRenderState() //since last render
{
  activeEditor()->contentsRendered = false;     //since last render
  activeEditor()->parameterWidget->setEnabled(false);
}

void TabManager::stopAnimation()
{
  par->animateWidget->pauseAnimation();
  par->animateWidget->e_tval->setText("");
}

void TabManager::updateFindState()
{
  switch (activeEditor()->findState) {
  case TabManager::FIND_REPLACE_VISIBLE: par->showFind(true); break;
  case TabManager::FIND_VISIBLE: par->showFind(false); break;
  default: par->hideFind(); break;
  }
}

void TabManager::onTabModified(EditorInterface *edt)
{
  // Get the name of the editor and its filepath with the status modifier
  auto [fname, fpath] = getEditorTabNameWithModifier(edt);

  // and set the tab bar widget.
  setEditorTabName(fname, fpath, edt);
}

void TabManager::openTabFile(const QString& filename)
{
#ifdef ENABLE_PYTHON
  if (boost::algorithm::ends_with(filename, ".py")) {
    std::string templ = "from openscad import *\n";
  } else
#endif
  QFileInfo fileinfo(filename);
  const auto suffix = fileinfo.suffix().toLower();
  const auto knownFileType = Importer::knownFileExtensions.contains(suffix);
  if (!knownFileType) return;

  const auto cmd = Importer::knownFileExtensions[suffix];
  if (cmd.isEmpty()) {
    activeEditor()->filepath = fileinfo.absoluteFilePath();
    refreshDocument();
    activeEditor()->parameterWidget->readFile(fileinfo.absoluteFilePath());
    par->updateRecentFiles(filename);
  } else {
    activeEditor()->filepath = "";
    activeEditor()->setPlainText(cmd.arg(filename));
  }

  emit editorContentReloaded(activeEditor());

  auto [fname, fpath] = getEditorTabNameWithModifier(activeEditor());
  setEditorTabName(fname, fpath, activeEditor());
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
    fname = "Untitled.scad";
    fpath = "Untitled.scad";
  }
  return {fname, fpath};
}

std::tuple<QString, QString> TabManager::getEditorTabNameWithModifier(EditorInterface *edt)
{
  auto [fname, fpath] = getEditorTabName(edt);

  // Add the "modification" star if it was changed.
  bool isDirty = edt->isContentModified()
    || edt->parameterWidget->isModified();

  if (isDirty) fname += "*";

  return {fname, fpath};
}

void TabManager::setEditorTabName(const QString& tabName, const QString& tabToolTip,
                                  EditorInterface *edt)
{
  int index = tabWidget->indexOf(edt);
  QString name = QString(tabName).replace("&", "&&");
  tabWidget->setTabText(index, name);
  tabWidget->setTabToolTip(index, tabToolTip);

  if (edt == activeEditor() ) emit editorNameChanged(name);
}

bool TabManager::refreshDocument()
{
  bool file_opened = false;
  if (!activeEditor()->filepath.isEmpty()) {
    QFile file(activeEditor()->filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      LOG("Failed to open file %1$s: %2$s",
          activeEditor()->filepath.toLocal8Bit().constData(), file.errorString().toLocal8Bit().constData());
    } else {
      QTextStream reader(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      reader.setCodec("UTF-8");
#endif
      auto text = reader.readAll();
      LOG("Loaded design '%1$s'.", activeEditor()->filepath.toLocal8Bit().constData());
      if (activeEditor()->toPlainText() != text) {
        activeEditor()->setPlainText(text);
        setContentRenderState();         // since last render
      }
      file_opened = true;
    }
  }
  return file_opened;
}

bool TabManager::maybeSave(int x)
{
  auto *edt = (EditorInterface *) tabWidget->widget(x);
  if (edt->isContentModified() || edt->parameterWidget->isModified()) {
    QMessageBox box(par);
    box.setText(_("The document has been modified."));
    box.setInformativeText(_("Do you want to save your changes?"));
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    box.setIcon(QMessageBox::Warning);
    box.setWindowModality(Qt::ApplicationModal);
#ifdef Q_OS_MACOS
    // Cmd-D is the standard shortcut for this button on Mac
    box.button(QMessageBox::Discard)->setShortcut(QKeySequence("Ctrl+D"));
    box.button(QMessageBox::Discard)->setShortcutEnabled(true);
#endif
    auto ret = (QMessageBox::StandardButton) box.exec();

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
  for (auto editor : editors()) {
    if (!(editor->isContentModified() || editor->parameterWidget->isModified())) continue;

    QMessageBox box(par);
    box.setText(_("Some tabs have unsaved changes."));
    box.setInformativeText(_("Do you want to save all your changes?"));
    box.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::SaveAll);
    box.setIcon(QMessageBox::Warning);
    box.setWindowModality(Qt::ApplicationModal);
#ifdef Q_OS_MACOS
    // Cmd-D is the standard shortcut for this button on Mac
    box.button(QMessageBox::Discard)->setShortcut(QKeySequence("Ctrl+D"));
    box.button(QMessageBox::Discard)->setShortcutEnabled(true);
#endif
    auto ret = (QMessageBox::StandardButton) box.exec();

    if (ret == QMessageBox::Cancel) {
      return false;
    } else if (ret == QMessageBox::Discard) {
      return true;
    } else if (ret == QMessageBox::SaveAll) {
      return saveAll();
    }
  }
  return true;
}

void TabManager::saveError(const QIODevice& file, const std::string& msg, const QString& filepath)
{
  const std::string fileName = filepath.toStdString();
  LOG("%1$s %2$s (%3$s)", msg.c_str(), fileName, file.errorString().toStdString());

  const std::string dialogFormatStr = msg + "\n\"%1\"\n(%2)";
  const QString dialogFormat(dialogFormatStr.c_str());
  QMessageBox::warning(par, par->windowTitle(), dialogFormat.arg(filepath).arg(file.errorString()));
}

/*!
 * Save current document.
 * Should _always_ write to disk, since this is called by SaveAs - i.e. don't
 * try to be smart and check for document modification here.
 */
bool TabManager::save(EditorInterface *edt)
{
  assert(edt != nullptr);

  if (edt->filepath.isEmpty()) {
    return saveAs(edt);
  } else {
    return save(edt, edt->filepath);
  }
}

bool TabManager::save(EditorInterface *edt, const QString& path)
{
  par->setCurrentOutput();

  // If available (>= Qt 5.1), use QSaveFile to ensure the file is not
  // destroyed if the device is full. Unfortunately this is not working
  // as advertised (at least in Qt 5.3) as it does not detect the device
  // full properly and happily commits a 0 byte file.
  // Checking the QTextStream status flag after flush() seems to catch
  // this condition.
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
    LOG("Saved design '%1$s'.", path.toLocal8Bit().constData());
    edt->parameterWidget->saveFile(path);
    edt->setContentModified(false);
    edt->parameterWidget->setModified(false);
    par->updateRecentFiles(path);
    edt->filepath = path;
  } else {
    saveError(file, _("Error saving design"), path);
  }
  return saveOk;
}

bool TabManager::saveAs(EditorInterface *edt)
{
  assert(edt != nullptr);

  const auto dir = edt->filepath.isEmpty() ? _("Untitled.scad") : edt->filepath;
  auto filename = QFileDialog::getSaveFileName(par, _("Save File"), dir, _("OpenSCAD Designs (*.scad)"));
  if (filename.isEmpty()) {
    return false;
  }

  if (QFileInfo(filename).suffix().isEmpty()) {
    filename.append(".scad");

    // Manual overwrite check since Qt doesn't do it, when using the
    // defaultSuffix property
    const QFileInfo info(filename);
    if (info.exists()) {
      const auto text = QString(_("%1 already exists.\nDo you want to replace it?")).arg(info.fileName());
      if (QMessageBox::warning(par, par->windowTitle(), text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return false;
      }
    }
  }

  bool saveOk = save(edt, filename);
  if (saveOk) {
    auto [fname, fpath] = getEditorTabNameWithModifier(edt);
    setEditorTabName(fname, fpath, edt);
    par->setWindowTitle(fname);
  }
  return saveOk;
}

bool TabManager::saveACopy(EditorInterface *edt)
{
  assert(edt != nullptr);

  const auto dir = edt->filepath.isEmpty() ? _("Untitled.scad") : edt->filepath;
  auto filename = QFileDialog::getSaveFileName(par, _("Save a Copy"), dir, _("OpenSCAD Designs (*.scad)"));
  if (filename.isEmpty()) {
    return false;
  }

  if (QFileInfo(filename).suffix().isEmpty()) {
    filename.append(".scad");
  }

  return save(edt, filename);
}

bool TabManager::saveAll()
{
  for (auto editor : editors()) {
    if (editor->isContentModified() || editor->parameterWidget->isModified()) {
      if (!save(editor)) {
        return false;
      }
    }
  }
  return true;
}
