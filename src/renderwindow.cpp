#include "renderwindow.h"
#include "ui_renderwindow.h"

renderWindow::renderWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::renderWindow)
{
    ui->setupUi(this);
        grabbedWidget = 0;
}

renderWindow::~renderWindow()
{
    delete ui;
}

void renderWindow::closeEvent(QCloseEvent *event)
{
        ui->horizontalLayout->removeWidget(grabbedWidget);
        emit passWidget(grabbedWidget);
        grabbedWidget = 0;
        QMainWindow::closeEvent(event);
}

void renderWindow::receiveWidget(QGLWidget *newglview)
{
        grabbedWidget = newglview;
        ui->horizontalLayout->addWidget(grabbedWidget);
}
