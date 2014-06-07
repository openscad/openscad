#ifndef SCINTILLAEDITOR_H
#define SCINTILLAEDITOR_H

#include <QObject>
#include <QWidget>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>

#include "editor.h"

class ScintillaEditor : public Editor
{
public:
    ScintillaEditor(QWidget *parent);
    QsciScintilla *qsci;
	QVBoxLayout *layout;
};

#endif // SCINTILLAEDITOR_H
