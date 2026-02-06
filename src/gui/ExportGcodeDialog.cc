#include "ExportGcodeDialog.h"
#include <QColorDialog>
#include <QPushButton>

ExportGcodeDialog::ExportGcodeDialog()
{
  setupUi(this);
  this->laserSpeed=Settings::SettingsExportGcode::exportGcodeFeedRate.value();
  this->laserPower=Settings::SettingsExportGcode::exportGcodeLaserPower.value();
  this->laserMode =Settings::SettingsExportGcode::exportGcodeLaserMode.value();
  valueLaserSpeed->setText(QString::number(this->laserSpeed));
  valueLaserPower->setText(QString::number(this->laserPower));
  valueLaserMode->setCurrentIndex(this->laserMode);

}

int ExportGcodeDialog::exec()
{
  bool showDialog = Settings::SettingsExportGcode::exportGcodeAlwaysShowDialog.value();
  if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
    showDialog = true;
  }
   return showDialog ? QDialog::exec() : QDialog::Accepted;
	
}

double ExportGcodeDialog::getLaserSpeed() const
{
  return laserSpeed;
}
double ExportGcodeDialog::getLaserPower() const
{
  return laserPower;
}

int ExportGcodeDialog::getLaserMode()  const
{
  return laserMode;
}

ExportGcodeOptions ExportGcodeDialog::getOptions()
{
  ExportGcodeOptions opts;
  opts.feedrate = getLaserSpeed();
  opts.laserpower = getLaserPower();
  opts.lasermode = getLaserMode();
  Settings::SettingsExportGcode::exportGcodeFeedRate.setValue(opts.feedrate);
  Settings::SettingsExportGcode::exportGcodeLaserPower.setValue(opts.laserpower);
  Settings::SettingsExportGcode::exportGcodeLaserMode.setValue(opts.lasermode);
  writeSettings();
  return opts;
}


void ExportGcodeDialog::on_valueLaserSpeed_textChanged(const QString& str)
{
  this->laserSpeed  = str.toDouble();	

}
void ExportGcodeDialog::on_valueLaserPower_textChanged(const QString& str)
{
  this->laserPower = str.toDouble();	

}
void ExportGcodeDialog::on_valueLaserMode_activated(int ind)
{
  this->laserMode = ind;	

}

void ExportGcodeDialog::on_pushButtonCancel_clicked()
{
  reject();
}

