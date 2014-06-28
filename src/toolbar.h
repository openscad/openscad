#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>
#include <QToolButton>
#include <QAction>
class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit ToolBar(QWidget *parent = 0);
    QToolButton *buttonZoomIn, *buttonZoomOut, *buttonRender;
    QToolButton *buttonTop, *buttonBottom, *buttonLeft, *buttonRight;
    QToolButton *buttonFront, *buttonBack, *buttonWireframe, *buttonSurface;

signals:

public slots:

};

#endif // TOOLBAR_H
