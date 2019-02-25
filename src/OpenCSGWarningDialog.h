#pragma once

#include "qtgettext.h"
#include "ui_OpenCSGWarningDialog.h"

class OpenCSGWarningDialog : public QDialog, public Ui::OpenCSGWarningDialog
{
  Q_OBJECT;
public:
  OpenCSGWarningDialog(QWidget *parent);

public slots:
  void setText(const QString &text);
};
