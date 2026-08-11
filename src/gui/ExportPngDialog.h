#pragma once

#include <QDialog>

#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ExportPngDialog.h"

class ExportPngDialog : public QDialog, public Ui::ExportPngDialog
{
  Q_OBJECT;

public:
  ExportPngDialog();

  bool isTransparentBackground() const;

private slots:
  void on_pushButtonOk_clicked();
  void on_pushButtonCancel_clicked();
};
