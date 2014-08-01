#include "launchingscreen.h"
#include "ui_launchingscreen.h"

LaunchingScreen::LaunchingScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LaunchingScreen)
{
   ui->setupUi(this);
   this->setStyleSheet("QDialog {background-image:url(':/background.png')}"
                       "QPushButton {color:white;}"
		       "QTreeWidget {background-color:rgba(255,255,255,0%)}");
   QPixmap pixmap(":/button.png");
   QPalette palette;

   palette.setBrush(ui->pushButtonNew->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->pushButtonOpen->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->pushButtonHelp->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->openRecentbtn->backgroundRole(), QBrush(pixmap));

   palette.setBrush(ui->exampleBtn->backgroundRole(), QBrush(pixmap));

   ui->pushButtonNew->setFlat(true);
   ui->pushButtonNew->setAutoFillBackground(true);
   ui->pushButtonNew->setPalette(palette);

   ui->pushButtonOpen->setFlat(true);
   ui->pushButtonOpen->setAutoFillBackground(true);
   ui->pushButtonOpen->setPalette(palette);

   ui->pushButtonHelp->setFlat(true);
   ui->pushButtonHelp->setAutoFillBackground(true);
   ui->pushButtonHelp->setPalette(palette);
 
   ui->openRecentbtn->setFlat(true);
   ui->openRecentbtn->setAutoFillBackground(true);
   ui->openRecentbtn->setPalette(palette);
 
   ui->exampleBtn->setFlat(true);
   ui->exampleBtn->setAutoFillBackground(true);
   ui->exampleBtn->setPalette(palette);


   ui->treeWidget->setFrameShape(QFrame::NoFrame);      
   //ui->treeWidget->headerItem()->setForeground(0, Qt::green); 
}

LaunchingScreen::~LaunchingScreen()
{
    delete ui;
}


