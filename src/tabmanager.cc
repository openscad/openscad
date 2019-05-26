#include <QTabWidget>
#include "editor.h"
#include "tabmanager.h"
#ifdef USE_SCINTILLA_EDITOR
#include "scintillaeditor.h"
#endif
#include "Preferences.h"
#include "MainWindow.h"


TabManager::TabManager(MainWindow *o)
{
    par = o;
    tabobj = new QTabWidget();
    tabobj->setTabsClosable(true);
    tabobj->setMovable(true);
    connect(tabobj, SIGNAL(currentChanged(int)), this, SLOT(curChanged(int)));
    connect(tabobj, SIGNAL(tabCloseRequested(int)), this, SLOT(closeRequested(int)));

    createTab();
}

QTabWidget *TabManager::getTabObj()
{
    return tabobj;
}

void TabManager::curChanged(int x)
{
    // std::cout << "current tab changed" << std::endl;
    editor = (EditorInterface *)tabobj->widget(x);
}

void TabManager::closeRequested(int x)
{
    QWidget *temp = tabobj->widget(x);
    tabobj->removeTab(x);

    // todo: popup dialog for saving of contents

    delete temp;
}

void TabManager::createTab()
{
    assert(par != nullptr);

    editor = new ScintillaEditor(tabobj);

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Preferences::create(editor->colorSchemes()); // needs to be done only once, however handled
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    connect(editor, SIGNAL(previewRequest()), par, SLOT(actionRenderPreview()));
    connect(Preferences::inst(), SIGNAL(editorConfigChanged()), editor, SLOT(applySettings()));
    Preferences::inst()->fireEditorConfigChanged();

    connect(par, SIGNAL(highlightError(int)), editor, SLOT(highlightError(int)));
    connect(par, SIGNAL(unhighlightLastError()), editor, SLOT(unhighlightLastError()));

    // Edit menu
    connect(par->editActionUndo, SIGNAL(triggered()), editor, SLOT(undo()));
    connect(editor, SIGNAL(contentsChanged()), par, SLOT(updateActionUndoState()));
    connect(par->editActionRedo, SIGNAL(triggered()), editor, SLOT(redo()));
    connect(par->editActionRedo_2, SIGNAL(triggered()), editor, SLOT(redo()));
    connect(par->editActionCut, SIGNAL(triggered()), editor, SLOT(cut()));
    connect(par->editActionCopy, SIGNAL(triggered()), editor, SLOT(copy()));
    connect(par->editActionPaste, SIGNAL(triggered()), editor, SLOT(paste()));

    connect(par->editActionIndent, SIGNAL(triggered()), editor, SLOT(indentSelection()));
    connect(par->editActionUnindent, SIGNAL(triggered()), editor, SLOT(unindentSelection()));
    connect(par->editActionComment, SIGNAL(triggered()), editor, SLOT(commentSelection()));
    connect(par->editActionUncomment, SIGNAL(triggered()), editor, SLOT(uncommentSelection()));
    connect(par->editActionZoomTextIn, SIGNAL(triggered()), editor, SLOT(zoomIn()));
    connect(par->editActionZoomTextOut, SIGNAL(triggered()), editor, SLOT(zoomOut()));

    connect(editor, SIGNAL(contentsChanged()), par, SLOT(animateUpdateDocChanged()));
    connect(editor, SIGNAL(contentsChanged()), par, SLOT(setContentsChanged()));
    connect(editor, SIGNAL(modificationChanged(bool)), par, SLOT(setWindowModified(bool)));

    connect(Preferences::inst(), SIGNAL(fontChanged(const QString&,uint)),
                    editor, SLOT(initFont(const QString&,uint)));
    connect(Preferences::inst(), SIGNAL(syntaxHighlightChanged(const QString&)),
                    editor, SLOT(setHighlightScheme(const QString&)));

    Preferences::inst()->apply_tab();



    tabobj->addTab(editor, "tab");
    tabobj->setCurrentWidget(editor);
}

void TabManager::curContent()
{
    std::cout << editor->toPlainText().toStdString() << std::endl << std::endl;

    // todo: set window title
}
