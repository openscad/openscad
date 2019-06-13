#include <QTabWidget>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include "editor.h"
#include "tabmanager.h"
#include "scintillaeditor.h"
#include "QSettingsCached.h"
#include "Preferences.h"
#include "MainWindow.h"
#include <Qsci/qscicommand.h>
#include <Qsci/qscicommandset.h>


TabManager::TabManager(MainWindow *o, const QString &filename)
{
    par = o;

    tabWidget = new QTabWidget();
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setTabBarAutoHide(true);
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSwitched(int)));
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTabRequested(int)));

    createTab(filename);

    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(stopAnimation()));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateFindState()));

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

QTabWidget *TabManager::getTabWidget()
{
    assert(tabWidget != nullptr);
    return tabWidget;
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

    // par->parameterWidget->setEnabled(editor->parameterWidgetState);
    par->editActionUndo->setEnabled(editor->canUndo());
    // par->setWindowModified(editor->isContentModified()); //can also emit signal instead
    par->editorTopLevelChanged(par->editorDock->isFloating());
    par->changedTopLevelConsole(par->consoleDock->isFloating());
    par->parameterTopLevelChanged(par->parameterDock->isFloating());
    par->setWindowTitle(tabWidget->tabText(x));
}

void TabManager::closeTabRequested(int x)
{
    assert(tabWidget != nullptr);
    if(!maybeSave(x))
        return;

    QWidget *temp = tabWidget->widget(x);
    editorList.remove((EditorInterface *)temp);
    tabWidget->removeTab(x);

    // todo: popup dialog for saving of contents

    delete temp;
}

void TabManager::actionNew()
{
    createTab("");
}

void TabManager::actionOpen()
{
    auto fileInfo = UIUtils::openFile(par);
    if (!fileInfo.exists()) {
        return;
    }

    if(editor->filepath.isEmpty() && !editor->isContentModified())
    {
        editor->filepath = fileInfo.filePath();
        refreshDocument();
    }
    else
    {
        createTab(fileInfo.filePath());
    }
}

void TabManager::createTab(const QString &filename)
{
    assert(par != nullptr);

    editor = new ScintillaEditor(tabWidget);
    par->activeEditor = editor;
    editorList.insert(editor);

    // clearing default mapping of keyboard shortcut for font size
    QsciCommandSet *qcmdset = ((ScintillaEditor *)editor)->qsci->standardCommands();
    QsciCommand *qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Plus);
    qcmd->setKey(0);
    qcmd = qcmdset->boundTo(Qt::ControlModifier | Qt::Key_Minus);
    qcmd->setKey(0);

    Preferences::create(editor->colorSchemes()); // needs to be done only once, however handled

    connect(editor, SIGNAL(previewRequest()), par, SLOT(actionRenderPreview()));
    connect(Preferences::inst(), SIGNAL(editorConfigChanged()), editor, SLOT(applySettings()));
    ((ScintillaEditor *)editor)->public_applySettings();

    connect(par->editActionZoomTextIn, SIGNAL(triggered()), editor, SLOT(zoomIn()));
    connect(par->editActionZoomTextOut, SIGNAL(triggered()), editor, SLOT(zoomOut()));

    connect(editor, SIGNAL(contentsChanged()), this, SLOT(updateActionUndoState())); 
    connect(editor, SIGNAL(contentsChanged()), par, SLOT(animateUpdateDocChanged())); 
    connect(editor, SIGNAL(contentsChanged()), this, SLOT(setContentRenderState()));
    // connect(editor, SIGNAL(modificationChanged(bool)), par, SLOT(setWindowModified(bool)));
    connect(editor, SIGNAL(modificationChanged(bool)), this, SLOT(setTabModified(bool)));

    connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)),
                    editor, SLOT(initFont(const QString&,uint)));
    connect(Preferences::inst(), SIGNAL(syntaxHighlightChanged(const QString&)),
                    editor, SLOT(setHighlightScheme(const QString&)));
    editor->initFont(Preferences::inst()->getValue("editor/fontfamily").toString(), Preferences::inst()->getValue("editor/fontsize").toUInt());
    editor->setHighlightScheme(Preferences::inst()->getValue("editor/syntaxhighlight").toString());

    if (!filename.isEmpty()) {
        openFileTab(filename);
    } else {
        setTab("");
    }
    par->updateRecentFileActions();
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

void TabManager::setTabModified(bool mod)
{
    QString fname = _("Untitled.scad");
    QString fpath = fname;
    if(!editor->filepath.isEmpty())
    {
        QFileInfo fileinfo(editor->filepath);
        fname = fileinfo.fileName();
        fpath = fileinfo.filePath();
    }
    if(mod)
    {
        fname += "*";
    }

    tabWidget->setTabText(tabWidget->currentIndex(), fname);
    tabWidget->setTabToolTip(tabWidget->currentIndex(), fpath);
    par->setWindowTitle(fname);
}

void TabManager::openFileTab(const QString &filename)
{
    par->setCurrentOutput();
    QFileInfo fileinfo(filename);
    const auto suffix = fileinfo.suffix().toLower();
    const auto knownFileType = par->knownFileExtensions.contains(suffix);
    const auto cmd = par->knownFileExtensions[suffix];
    if (knownFileType && cmd.isEmpty()) {
        setTab(filename);
        par->updateRecentFiles();
    } else {
        setTab(nullptr);
        editor->setPlainText(cmd.arg(filename));
    }
    //fileChangedOnDisk(); // force cached autoReloadId to update
    this->refreshDocument();
   // clearExportPaths();

    par->hideCurrentOutput(); // Initial parse for customizer, hide any errors to avoid duplication
    // try {
    //     parseTopLevelDocument(true);
    // } catch (const HardWarningException&) {
    //     exceptionCleanup();
    // }
    // this->last_compiled_doc = ""; // undo the damage so F4 works
    par->clearCurrentOutput();
}

void TabManager::setTab(const QString &filename)
{
    if (filename.isEmpty()) {
        tabWidget->addTab(editor, _("Untitled.scad"));
        tabWidget->setCurrentWidget(editor);
        tabWidget->setTabToolTip(tabWidget->currentIndex(), _("Untitled.scad"));
    } else {
        QFileInfo fileinfo(filename);
        editor->filepath = fileinfo.absoluteFilePath();
        tabWidget->addTab(editor, fileinfo.fileName());
        tabWidget->setCurrentWidget(editor);
        tabWidget->setTabToolTip(tabWidget->currentIndex(), fileinfo.filePath());
        par->parameterWidget->readFile(editor->filepath);  ////////////////////////////////
        QDir::setCurrent(fileinfo.dir().absolutePath());
        // this->top_ctx.setDocumentPath(fileinfo.dir().absolutePath().toLocal8Bit().constData());
    }
    par->editorTopLevelChanged(par->editorDock->isFloating());
    par->changedTopLevelConsole(par->consoleDock->isFloating());
    par->parameterTopLevelChanged(par->parameterDock->isFloating());
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
                setContentRenderState(); // since last render; should not be here
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
            par->actionSave();
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
        box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::Save);
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
    }
    return true;
}
