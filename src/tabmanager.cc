#include <QTabWidget>
#include "editor.h"
#include "tabmanager.h"
// #include "legacyeditor.h"
// #ifdef USE_SCINTILLA_EDITOR
#include "scintillaeditor.h"
// #endif
#include "QSettingsCached.h"
#include "Preferences.h"
#include "MainWindow.h"
#include <Qsci/qscicommand.h>
#include <Qsci/qscicommandset.h>


TabManager::TabManager(MainWindow *o)
{
    par = o;

    QSettingsCached settings;
    editortype = settings.value(Preferences::PREF_EDITOR_TYPE).toString();
    useMultitab = (editortype == Preferences::EDITOR_TYPE_MULTITAB);
    par->fileActionNewTab->setEnabled(useMultitab);

    if(useMultitab)
    {
        tabobj = new QTabWidget();
        tabobj->setTabsClosable(true);
        tabobj->setMovable(true);
        connect(tabobj, SIGNAL(currentChanged(int)), this, SLOT(curChanged(int)));
        connect(tabobj, SIGNAL(tabCloseRequested(int)), this, SLOT(closeRequested(int)));
    }

    createTab();

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

QTabWidget *TabManager::getTabObj()
{
    assert(tabobj != nullptr);
    return tabobj;
}

void TabManager::curChanged(int x)
{
    assert(tabobj != nullptr);
    editor = (EditorInterface *)tabobj->widget(x);

    par->editActionUndo->setEnabled(editor->canUndo());
    par->setWindowModified(editor->isContentModified()); //can also emit signal instead
}

void TabManager::closeRequested(int x)
{
    assert(tabobj != nullptr);
    QWidget *temp = tabobj->widget(x);
    tabobj->removeTab(x);

    // todo: popup dialog for saving of contents

    delete temp;
}

void TabManager::createTab()
{
    assert(par != nullptr);

    if(useMultitab)
        editor = new ScintillaEditor(tabobj);
    else 
        editor = new ScintillaEditor(par->editorDockContents);

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
    connect(editor, SIGNAL(contentsChanged()), par, SLOT(setContentsChanged()));
    connect(editor, SIGNAL(modificationChanged(bool)), par, SLOT(setWindowModified(bool)));

    connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)),
                    editor, SLOT(initFont(const QString&,uint)));
    connect(Preferences::inst(), SIGNAL(syntaxHighlightChanged(const QString&)),
                    editor, SLOT(setHighlightScheme(const QString&)));
    editor->initFont(Preferences::inst()->getValue("editor/fontfamily").toString(), Preferences::inst()->getValue("editor/fontsize").toUInt());
    editor->setHighlightScheme(Preferences::inst()->getValue("editor/syntaxhighlight").toString());

    if(useMultitab)
    {
        tabobj->addTab(editor, "tab");
        tabobj->setCurrentWidget(editor);
    }
}

void TabManager::curContent()
{
    std::cout << editor->toPlainText().toStdString() << std::endl << std::endl;

    // todo: set window title
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
