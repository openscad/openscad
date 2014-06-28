#include "toolbar.h"

ToolBar::ToolBar(QWidget *parent) :
    QToolBar(parent)
{
    int defaultColor = this->palette().background().color().lightness();
    
    buttonRender = new QToolButton;
    buttonTop = new QToolButton;
    buttonBottom = new QToolButton;
    buttonLeft = new QToolButton;
    buttonRight = new QToolButton;
    buttonFront = new QToolButton;
    buttonBack = new QToolButton;
    buttonZoomIn = new QToolButton;
    buttonZoomOut = new QToolButton;
    buttonWireframe = new QToolButton;
    buttonSurface = new QToolButton;
   
    if(defaultColor > 165)
    {
        buttonRender->setIcon(QIcon("://images/blackRender.png"));
	buttonRender->setToolTip("Render");
        buttonTop->setIcon(QIcon("://images/blackUp.png"));
	buttonBottom->setIcon(QIcon("://images/blackbottom.png"));
        buttonLeft->setIcon(QIcon("://images/blackleft (copy).png"));
        buttonRight->setIcon(QIcon("://images/rightright.png"));
        buttonFront->setIcon(QIcon("://images/blackfront.png"));
        buttonBack->setIcon(QIcon("://images/blackback.png"));
	buttonZoomIn->setIcon(QIcon("://images/zoomin.png"));
	buttonZoomOut->setIcon(QIcon("://images/zoomout.png"));
	buttonWireframe->setIcon(QIcon("://images/wireframe1.png"));
	buttonSurface->setIcon(QIcon("://images/surface.png"));
     } else {
	
        buttonRender->setIcon(QIcon("://images/Arrowhead-Right-32.png"));
        buttonTop->setIcon(QIcon("://images/up.png"));
        buttonBottom->setIcon(QIcon("://images/bottom.png"));
        buttonLeft->setIcon(QIcon("://images/left.png"));
        buttonRight->setIcon(QIcon("://images/right.png"));
        buttonFront->setIcon(QIcon("://images/front.png"));
        buttonBack->setIcon(QIcon("://images/back.png"));

    }
	buttonRender->setToolTip("Render");
	buttonTop->setToolTip("Top");
	buttonBottom->setToolTip("Bottom");
	buttonLeft->setToolTip("Left");
	buttonRight->setToolTip("Right");
	buttonFront->setToolTip("Front");
	buttonBack->setToolTip("Back");
	buttonZoomIn->setToolTip("Zoom in");
	buttonZoomOut->setToolTip("Zoom out");
        buttonWireframe->setToolTip("Wireframe");
	buttonSurface->setToolTip("Surfaces");
	
    this->addWidget(buttonRender);
    this->addSeparator();
    this->addWidget(buttonTop);
    this->addWidget(buttonBottom);
    this->addWidget(buttonLeft);
    this->addWidget(buttonRight);
    this->addWidget(buttonFront);
    this->addWidget(buttonBack);
    this->addWidget(buttonZoomIn);
    this->addWidget(buttonZoomOut);
    this->addWidget(buttonWireframe);
    this->addWidget(buttonSurface);
}
