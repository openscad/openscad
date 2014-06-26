#ifndef EDITORTOOLBAR_H
#define EDITORTOOLBAR_H

#include <QToolBar>
#include <QToolButton>

class EditorToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit EditorToolBar(QWidget *parent = 0);
    QToolButton *buttonNew, *buttonOpen, *buttonSave;
    QToolButton *buttonZoomIn, *buttonZoomOut;

signals:

public slots:

};

#endif // EDITORTOOLBAR_H
