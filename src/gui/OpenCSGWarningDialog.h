#pragma once

#include <QDialog>
#include <QWidget>

#include "gui/qtgettext.h"
#include "ui_OpenCSGWarningDialog.h"

class OpenCSGWarningDialog : public QDialog, public Ui::OpenCSGWarningDialog
{
  Q_OBJECT;

public:
  OpenCSGWarningDialog(QWidget *parent);

public slots:
  void setText(const QString& text);

private slots:
  void on_showBox_toggled(bool checked);
};
