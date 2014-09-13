#include "launchingscreen.h"
#include "ui_launchingscreen.h"

LaunchingScreen::LaunchingScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaunchingScreen)
{
   ui->setupUi(this);
   this->setStyleSheet("QDialog {background-image:url(':/icons/background.png')}"
                       "QPushButton {color:white;}");
}

LaunchingScreen::~LaunchingScreen()
{
    delete ui;
}


