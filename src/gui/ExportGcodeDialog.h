#pragma once

#include <memory>
#include <QDialog>
#include <QColor>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ExportGcodeDialog.h"
#include "gui/InitConfigurator.h"
#include "io/export.h"

class ExportGcodeDialog : public QDialog, public Ui::ExportGcodeDialog, public InitConfigurator
{
  Q_OBJECT;

public:
  ExportGcodeDialog();

  int exec() override;

  double getLaserSpeed() const;
  double getLaserPower() const;
  int getLaserMode() const;
  QString getInitCode() const;
  QString getExitCode() const;
  ExportGcodeOptions getOptions();

private slots:
  void on_valueLaserSpeed_textChanged(const QString&);
  void on_valueLaserPower_textChanged(const QString&);
  void on_valueInitCode_textChanged(void);
  void on_valueExitCode_textChanged(void);
  void on_valueLaserMode_activated(int);
  void on_pushButtonCancel_clicked();

private:
  double laserSpeed;
  double laserPower;
  int laserMode;
  QString initCode;
  QString exitCode;
};
