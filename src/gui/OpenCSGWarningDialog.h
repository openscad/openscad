#pragma once

#include "gui/qtgettext.h"
#include <QDialog>
#include <QWidget>
#include "ui_OpenCSGWarningDialog.h"

class OpenCSGWarningDialog : public QDialog, public Ui::OpenCSGWarningDialog
{
  Q_OBJECT;
public:
  OpenCSGWarningDialog(QWidget *parent);

public slots:
  void setText(const QString& text);
};
