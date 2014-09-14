#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>
#include <QToolButton>
class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit ToolBar(QWidget *parent = 0);
    QToolButton *buttonNew, *buttonOpen, *buttonSave, *buttonRender;
    QToolButton *buttonTop, *buttonBottom, *buttonLeft, *buttonRight;
    QToolButton *buttonFront, *buttonBack;

signals:

public slots:

};

#endif // TOOLBAR_H
