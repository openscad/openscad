#include "launchingscreen.h"
#include "ui_launchingscreen.h"

LaunchingScreen::LaunchingScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaunchingScreen)
{
   ui->setupUi(this);
   this->setStyleSheet("QDialog {background-image:url(':/background.png')}"
                       "QPushButton {color:white;}");
   QPixmap pixmap(":/button.png");
   QPalette palette;

   palette.setBrush(ui->pushButtonNew->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->pushButtonOpen->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->pushButtonHelp->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->openRecentButton->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->openExampleButton->backgroundRole(), QBrush(pixmap));

   ui->pushButtonNew->setFlat(true);
   ui->pushButtonNew->setAutoFillBackground(true);
   ui->pushButtonNew->setPalette(palette);

   ui->pushButtonOpen->setFlat(true);
   ui->pushButtonOpen->setAutoFillBackground(true);
   ui->pushButtonOpen->setPalette(palette);

   ui->pushButtonHelp->setFlat(true);
   ui->pushButtonHelp->setAutoFillBackground(true);
   ui->pushButtonHelp->setPalette(palette);
 
   ui->openRecentButton->setFlat(true);
   ui->openRecentButton->setAutoFillBackground(true);
   ui->openRecentButton->setPalette(palette);
 
   ui->openExampleButton->setFlat(true);
   ui->openExampleButton->setAutoFillBackground(true);
   ui->openExampleButton->setPalette(palette);


//   ui->treeWidget->setFrameShape(QFrame::NoFrame);      
   //ui->treeWidget->headerItem()->setForeground(0, Qt::green); 
}

LaunchingScreen::~LaunchingScreen()
{
    delete ui;
}


