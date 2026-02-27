#include "ExportGcodeDialog.h"
#include <QColorDialog>
#include <QPushButton>

ExportGcodeDialog::ExportGcodeDialog()
{
  setupUi(this);
  this->laserSpeed = Settings::SettingsExportGcode::exportGcodeFeedRate.value();
  this->laserPower = Settings::SettingsExportGcode::exportGcodeLaserPower.value();
  this->laserMode = Settings::SettingsExportGcode::exportGcodeLaserMode.value();
  this->initCode = QString(Settings::SettingsExportGcode::exportGcodeInitCode.value().c_str());
  this->exitCode = QString(Settings::SettingsExportGcode::exportGcodeExitCode.value().c_str());
  valueLaserSpeed->setText(QString::number(this->laserSpeed));
  valueLaserPower->setText(QString::number(this->laserPower));
  valueInitCode->setText(this->initCode);
  valueExitCode->setText(this->exitCode);
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

int ExportGcodeDialog::getLaserMode() const
{
  return laserMode;
}

QString ExportGcodeDialog::getInitCode() const
{
  return initCode;
}

QString ExportGcodeDialog::getExitCode() const
{
  return exitCode;
}

ExportGcodeOptions ExportGcodeDialog::getOptions()
{
  ExportGcodeOptions opts;
  opts.feedrate = getLaserSpeed();
  opts.laserpower = getLaserPower();
  opts.lasermode = getLaserMode();
  opts.initCode = getInitCode().toStdString();
  opts.exitCode = getExitCode().toStdString();
  Settings::SettingsExportGcode::exportGcodeFeedRate.setValue(opts.feedrate);
  Settings::SettingsExportGcode::exportGcodeLaserPower.setValue(opts.laserpower);
  Settings::SettingsExportGcode::exportGcodeLaserMode.setValue(opts.lasermode);
  Settings::SettingsExportGcode::exportGcodeInitCode.setValue(opts.initCode);
  Settings::SettingsExportGcode::exportGcodeExitCode.setValue(opts.exitCode);
  writeSettings();
  return opts;
}

void ExportGcodeDialog::on_valueLaserSpeed_textChanged(const QString& str)
{
  this->laserSpeed = str.toDouble();
}
void ExportGcodeDialog::on_valueLaserPower_textChanged(const QString& str)
{
  this->laserPower = str.toDouble();
}

void ExportGcodeDialog::on_valueLaserMode_activated(int ind)
{
  this->laserMode = ind;
}

void ExportGcodeDialog::on_valueInitCode_textChanged(void)
{
  this->initCode = valueInitCode->toPlainText();
}

void ExportGcodeDialog::on_valueExitCode_textChanged(void)
{
  this->exitCode = valueExitCode->toPlainText();
}

void ExportGcodeDialog::on_pushButtonCancel_clicked()
{
  reject();
}
