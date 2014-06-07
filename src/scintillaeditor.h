#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <Qsci/qsciscintilla.h>

#include "editor.h"

class ScintillaEditor : public Editor
{
public:
    ScintillaEditor(QWidget *parent);
    QsciScintilla *qsci;
};

#endif // SCINTILLAEDITOR_H
