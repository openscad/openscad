#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QShortcut>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <Qsci/qscicommand.h>
#include <Qsci/qscicommandset.h>

#include "editor.h"
#include "tabmanager.h"
#include "tabwidget.h"
#include "scintillaeditor.h"
#include "QSettingsCached.h"
#include "Preferences.h"
#include "MainWindow.h"

TabManager::TabManager(MainWindow *o, const QString &filename)
{
    par = o;

    tabWidget = new TabWidget();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    tabWidget->setAutoHide(true);
#endif
    tabWidget->setExpanding(false);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    connect(tabWidget, SIGNAL(currentTabChanged(int)), this, SLOT(tabSwitched(int)));
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTabRequested(int)));
    connect(tabWidget, SIGNAL(tabCountChanged(int)), this, SIGNAL(tabCountChanged(int)));

    createTab(filename);

    connect(tabWidget, SIGNAL(currentTabChanged(int)), this, SLOT(stopAnimation()));
    connect(tabWidget, SIGNAL(currentTabChanged(int)), this, SLOT(updateFindState()));

    connect(par, SIGNAL(highlightError(int)), this, SLOT(highlightError(int)));
    connect(par, SIGNAL(unhighlightLastError()), this, SLOT(unhighlightLastError()));

    connect(par->editActionUndo, SIGNAL(triggered()), this, SLOT(undo()));
    connect(par->editActionRedo, SIGNAL(triggered()), this, SLOT(redo()));
    connect(par->editActionRedo_2, SIGNAL(triggered()), this, SLOT(redo()));
    connect(par->editActionCut, SIGNAL(triggered()), this, SLOT(cut()));
    connect(par->editActionCopy, SIGNAL(triggered()), this, SLOT(copy()));
    connect(par->editActionPaste, SIGNAL(triggered()), this, SLOT(paste()));

    connect(par->editActionIndent, SIGNAL(triggered()), this, SLOT(indentSelection()));
    connect(par->editActionUnindent, SIGNAL(triggered()), this, SLOT(unindentSelection()));
    connect(par->editActionComment, SIGNAL(triggered()), this, SLOT(commentSelection()));
    connect(par->editActionUncomment, SIGNAL(triggered()), this, SLOT(uncommentSelection()));
}

QWidget *TabManager::getTabHeader()
{
    assert(tabWidget != nullptr);
    return tabWidget;
}

QWidget *TabManager::getTabContent()
{
    assert(tabWidget != nullptr);
    return tabWidget->getContentWidget();
}

void TabManager::tabSwitched(int x)
{
    assert(tabWidget != nullptr);
    editor = (EditorInterface *)tabWidget->widget(x);
    par->activeEditor = editor;

    if(editor == par->customizerEditor)
    {
        par->parameterWidget->setEnabled(true);
    }
    else
    {
        par->parameterWidget->setEnabled(false);
    }

    par->editActionUndo->setEnabled(editor->canUndo());
    par->changedTopLevelEditor(par->editorDock->isFloating());
    par->changedTopLevelConsole(par->consoleDock->isFloating());
    par->parameterTopLevelChanged(par->parameterDock->isFloating());
    par->setWindowTitle(tabWidget->tabText(x).replace("&&", "&"));
}

void TabManager::closeTabRequested(int x)
{
    assert(tabWidget != nullptr);
    if(!maybeSave(x))
        return;

    QWidget *temp = tabWidget->widget(x);
    editorList.remove((EditorInterface *)temp);
    tabWidget->removeTab(x);
    tabWidget->fireTabCountChanged();

    delete temp;
}

void TabManager::actionNew()
{
    createTab("");
}

void TabManager::open(const QString &filename)
{
    assert(!filename.isEmpty());

    for(auto edt: editorList)
    {
        if(filename == edt->filepath)
        {
            tabWidget->setCurrentWidget(tabWidget->indexOf(edt));
            return;
        }
    }

    if(editor->filepath.isEmpty() && !editor->isContentModified())
    {
        openTabFile(filename);
    }
    else
    {
        createTab(filename);
    }
}

void TabManager::createTab(const QString &filename)
{
    assert(par != nullptr);

    editor = new ScintillaEditor(tabWidget);
    par->activeEditor = editor;

    // clearing default mapping of keyboard shortcut for font size
    QsciCommandSet *qcmdset = ((ScintillaEditor *)editor)->qsci->standardCommands();
    QsciCommand *qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Plus);
    qcmd->setKey(0);
    qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Minus);
    qcmd->setKey(0);

    Preferences::create(editor->colorSchemes()); // needs to be done only once, however handled

    connect(editor, SIGNAL(previewRequest()), par, SLOT(actionRenderPreview()));
    connect(Preferences::inst(), SIGNAL(editorConfigChanged()), editor, SLOT(applySettings()));
	connect(Preferences::inst(), SIGNAL(autocompleteChanged(bool)), editor, SLOT(onAutocompleteChanged(bool)));
	connect(Preferences::inst(), SIGNAL(characterThresholdChanged(int)), editor, SLOT(onCharacterThresholdChanged(int)));
    ((ScintillaEditor *)editor)->public_applySettings();
	editor->addTemplate();

	QShortcut *viewTemplates = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Insert), editor);
	viewTemplates->setAutoRepeat(false);
	connect(viewTemplates, SIGNAL(activated()), editor, SLOT(displayTemplates()));

    connect(par->editActionZoomTextIn, SIGNAL(triggered()), editor, SLOT(zoomIn()));
    connect(par->editActionZoomTextOut, SIGNAL(triggered()), editor, SLOT(zoomOut()));

    connect(editor, SIGNAL(contentsChanged()), this, SLOT(updateActionUndoState())); 
    connect(editor, SIGNAL(contentsChanged()), par, SLOT(animateUpdateDocChanged())); 
    connect(editor, SIGNAL(contentsChanged()), this, SLOT(setContentRenderState()));
    connect(editor, SIGNAL(modificationChanged(bool, EditorInterface *)), this, SLOT(setTabModified(bool, EditorInterface *)));

    connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)),
                    editor, SLOT(initFont(const QString&,uint)));
    connect(Preferences::inst(), SIGNAL(syntaxHighlightChanged(const QString&)),
                    editor, SLOT(setHighlightScheme(const QString&)));
    editor->initFont(Preferences::inst()->getValue("editor/fontfamily").toString(), Preferences::inst()->getValue("editor/fontsize").toUInt());
    editor->setHighlightScheme(Preferences::inst()->getValue("editor/syntaxhighlight").toString());

    int idx = tabWidget->addTab(editor, _("Untitled.scad"));
    if(!editorList.isEmpty()) {
        tabWidget->setCurrentWidget(idx); // to prevent emitting of currentTabChanged signal twice for first tab
    }

    editorList.insert(editor);
    if (!filename.isEmpty()) {
        openTabFile(filename);
    } else {
        setTabName("");
    }
    par->updateRecentFileActions();
}

int TabManager::count()
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

void TabManager::updateActionUndoState()
{
    par->editActionUndo->setEnabled(editor->canUndo());
}

void TabManager::setContentRenderState() //since last render
{
    editor->contentsRendered = false; //since last render
    if(editor == par->customizerEditor)
    {
        par->customizerEditor = nullptr;
        par->parameterWidget->setEnabled(false);
    }
}

void TabManager::stopAnimation()
{
    par->viewActionAnimate->setChecked(false);
    par->viewModeAnimate();
    par->e_tval->setText("");
}

void TabManager::updateFindState()
{
    if(editor->findState == TabManager::FIND_REPLACE_VISIBLE)
        par->showFindAndReplace();
    else if(editor->findState == TabManager::FIND_VISIBLE)
        par->showFind();
    else
        par->hideFind();
}

void TabManager::setTabModified(bool mod, EditorInterface *edt)
{
    QString fname = _("Untitled.scad");
    QString fpath = fname;
    if(!edt->filepath.isEmpty())
    {
        QFileInfo fileinfo(edt->filepath);
        fname = fileinfo.fileName();
        fpath = fileinfo.filePath();
    }
    if(mod)
    {
        fname += "*";
    }

    if(edt == editor) {
        par->setWindowTitle(fname);
    }
    tabWidget->setTabText(tabWidget->indexOf(edt), fname.replace("&", "&&"));
    tabWidget->setTabToolTip(tabWidget->indexOf(edt), fpath);
}

void TabManager::openTabFile(const QString &filename)
{
    par->setCurrentOutput();
    editor->setPlainText("");

    QFileInfo fileinfo(filename);
    const auto suffix = fileinfo.suffix().toLower();
    const auto knownFileType = par->knownFileExtensions.contains(suffix);
    const auto cmd = par->knownFileExtensions[suffix];
    if (knownFileType && cmd.isEmpty()) {
        setTabName(filename);
        par->updateRecentFiles(editor);
    } else {
        setTabName(nullptr);
        editor->setPlainText(cmd.arg(filename));
    }
    par->fileChangedOnDisk(); // force cached autoReloadId to update
    refreshDocument();

    par->hideCurrentOutput(); // Initial parse for customizer, hide any errors to avoid duplication
    try {
        par->parseTopLevelDocument(true);
    } catch (const HardWarningException&) {
        par->exceptionCleanup();
    }
    par->last_compiled_doc = ""; // undo the damage so F4 works
    par->clearCurrentOutput();
}

void TabManager::setTabName(const QString &filename, EditorInterface *edt)
{
    if(edt == nullptr) {
        edt = editor;
    }

    QString fname;
    if (filename.isEmpty()) {
        edt->filepath.clear();
        fname = _("Untitled.scad");
        tabWidget->setTabText(tabWidget->indexOf(edt), fname);
        tabWidget->setTabToolTip(tabWidget->indexOf(edt), fname);
    } else {
        QFileInfo fileinfo(filename);
        edt->filepath = fileinfo.absoluteFilePath();
        fname = fileinfo.fileName();
        tabWidget->setTabText(tabWidget->indexOf(edt), QString(fname).replace("&", "&&"));
        tabWidget->setTabToolTip(tabWidget->indexOf(edt), fileinfo.filePath());
        par->parameterWidget->readFile(edt->filepath);
        QDir::setCurrent(fileinfo.dir().absolutePath());
    }
    par->editorTopLevelChanged(par->editorDock->isFloating());
    par->changedTopLevelConsole(par->consoleDock->isFloating());
    par->parameterTopLevelChanged(par->parameterDock->isFloating());
    par->setWindowTitle(fname);
}

void TabManager::refreshDocument()
{
    par->setCurrentOutput();
    if (!editor->filepath.isEmpty()) {
        QFile file(editor->filepath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            PRINTB("Failed to open file %s: %s",
                         editor->filepath.toLocal8Bit().constData() % file.errorString().toLocal8Bit().constData());
        }
        else {
            QTextStream reader(&file);
            reader.setCodec("UTF-8");
            auto text = reader.readAll();
            PRINTB("Loaded design '%s'.", editor->filepath.toLocal8Bit().constData());
            if (editor->toPlainText() != text) {
                editor->setPlainText(text);
                setContentRenderState(); // since last render
            }
        }
    }
    par->setCurrentOutput();
}

bool TabManager::maybeSave(int x)
{
    EditorInterface *edt = (EditorInterface *)tabWidget->widget(x);
    if (edt->isContentModified()) {
        QMessageBox box(par);
        box.setText(_("The document has been modified."));
        box.setInformativeText(_("Do you want to save your changes?"));
        box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::Save);
        box.setIcon(QMessageBox::Warning);
        box.setWindowModality(Qt::ApplicationModal);
#ifdef Q_OS_MAC
        // Cmd-D is the standard shortcut for this button on Mac
        box.button(QMessageBox::Discard)->setShortcut(QKeySequence("Ctrl+D"));
        box.button(QMessageBox::Discard)->setShortcutEnabled(true);
#endif
        auto ret = (QMessageBox::StandardButton) box.exec();

        if (ret == QMessageBox::Save) {
            save(edt);
            // Returns false on failed save
            return !edt->isContentModified();
        }
        else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

bool TabManager::shouldClose()
{
    foreach(EditorInterface *edt, editorList)
    {
        if(!edt->isContentModified())
            continue;

        QMessageBox box(par);
        box.setText(_("Some of the tabs are modified."));
        box.setInformativeText(_("All unsaved changes will be lost."));
        box.setStandardButtons(QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::SaveAll);
        box.setIcon(QMessageBox::Warning);
        box.setWindowModality(Qt::ApplicationModal);
#ifdef Q_OS_MAC
        // Cmd-D is the standard shortcut for this button on Mac
        box.button(QMessageBox::Discard)->setShortcut(QKeySequence("Ctrl+D"));
        box.button(QMessageBox::Discard)->setShortcutEnabled(true);
#endif
        auto ret = (QMessageBox::StandardButton) box.exec();

        if (ret == QMessageBox::Cancel) {
            return false;
        }
        else if(ret == QMessageBox::Discard) {
            return true;
        }
        else if(ret == QMessageBox::SaveAll) {
            saveAll();
            return false;
        }
    }
    return true;
}

void TabManager::saveError(const QIODevice &file, const std::string &msg, EditorInterface *edt)
{
    const std::string messageFormat = msg + " %s (%s)";
    const char *fileName = edt->filepath.toLocal8Bit().constData();
    PRINTB(messageFormat.c_str(), fileName % file.errorString().toLocal8Bit().constData());

    const std::string dialogFormatStr = msg + "\n\"%1\"\n(%2)";
    const QString dialogFormat(dialogFormatStr.c_str());
    QMessageBox::warning(par, par->windowTitle(), dialogFormat.arg(edt->filepath).arg(file.errorString()));
}

/*!
    Save current document.
    Should _always_ write to disk, since this is called by SaveAs - i.e. don't try to be
    smart and check for document modification here.
 */
void TabManager::save(EditorInterface *edt)
{
    assert(edt != nullptr);

    if (edt->filepath.isEmpty()) {
        saveAs(edt);
        return;
    }

    par->setCurrentOutput();

    // If available (>= Qt 5.1), use QSaveFile to ensure the file is not
    // destroyed if the device is full. Unfortunately this is not working
    // as advertised (at least in Qt 5.3) as it does not detect the device
    // full properly and happily commits a 0 byte file.
    // Checking the QTextStream status flag after flush() seems to catch
    // this condition.
    QSaveFile file(edt->filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        saveError(file, _("Failed to open file for writing"), edt);
    }
    else {
        QTextStream writer(&file);
        writer.setCodec("UTF-8");
        writer << edt->toPlainText();
        writer.flush();
        bool saveOk = writer.status() == QTextStream::Ok;
	if (saveOk) { saveOk = file.commit(); } else { file.cancelWriting(); }
        if (saveOk) {
            PRINTB(_("Saved design '%s'."), edt->filepath.toLocal8Bit().constData());
            edt->setContentModified(false);
        } else {
            saveError(file, _("Error saving design"), edt);
        }
    }
    par->updateRecentFiles(edt);
}

void TabManager::saveAs(EditorInterface *edt)
{
    auto new_filename = QFileDialog::getSaveFileName(par, _("Save File"),
            edt->filepath.isEmpty()?_("Untitled.scad"):edt->filepath,
            _("OpenSCAD Designs (*.scad)"));
    if (!new_filename.isEmpty()) {
        if (QFileInfo(new_filename).suffix().isEmpty()) {
            new_filename.append(".scad");

            // Manual overwrite check since Qt doesn't do it, when using the
            // defaultSuffix property
            QFileInfo info(new_filename);
            if (info.exists()) {
                if (QMessageBox::warning(par, par->windowTitle(),
                                                                 QString(_("%1 already exists.\nDo you want to replace it?")).arg(info.fileName()),
                                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
                    return;
                }
            }
        }
        par->parameterWidget->writeFileIfNotEmpty(new_filename);
        setTabName(new_filename, edt);
        save(edt);
    }
}

void TabManager::saveAll()
{
    foreach(EditorInterface *edt, editorList) 
    {
        if(edt->isContentModified()) {
            save(edt);
        }
    }
}
