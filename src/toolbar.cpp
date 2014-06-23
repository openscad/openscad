#include "toolbar.h"

ToolBar::ToolBar(QWidget *parent) :
    QToolBar(parent)
{
    buttonNew = new QToolButton;
    buttonNew->setIcon(QIcon("://images/Document-New-128.png"));

    buttonOpen = new QToolButton;
    buttonOpen->setIcon(QIcon("://images/Open-128.png"));

    buttonSave = new QToolButton;
    buttonSave->setIcon(QIcon("://images/Save-128.png"));

    buttonRender = new QToolButton;
    buttonRender->setIcon(QIcon("://images/Arrowhead-Right-32.png"));

    buttonTop = new QToolButton;
    buttonTop->setIcon(QIcon("://images/up.png"));

    buttonBottom = new QToolButton;
    buttonBottom->setIcon(QIcon("://images/bottom.png"));

    buttonLeft = new QToolButton;
    buttonLeft->setIcon(QIcon("://images/left.png"));

    buttonRight = new QToolButton;
    buttonRight->setIcon(QIcon("://images/right.png"));

    buttonFront = new QToolButton;
    buttonFront->setIcon(QIcon("://images/front.png"));

    buttonBack = new QToolButton;
    buttonBack->setIcon(QIcon("://images/back.png"));

    this->addWidget(buttonNew);
    this->addWidget(buttonOpen);
    this->addWidget(buttonSave);
    this->addWidget(buttonRender);
    this->addSeparator();
    this->addWidget(buttonTop);
    this->addWidget(buttonBottom);
    this->addWidget(buttonLeft);
    this->addWidget(buttonRight);
    this->addWidget(buttonFront);
    this->addWidget(buttonBack);

}
