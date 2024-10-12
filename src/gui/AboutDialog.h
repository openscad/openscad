#pragma once

#include "version.h"
#include <QDialog>
#include <QWidget>
#include "gui/qtgettext.h"
#include "ui_AboutDialog.h"

class AboutDialog : public QDialog, public Ui::AboutDialog
{
  Q_OBJECT;
public:
  AboutDialog(QWidget *) {
    setupUi(this);
    this->setWindowTitle(QString(_("About OpenSCAD")) + " " + openscad_shortversionnumber.c_str());
    QString tmp = this->aboutText->toHtml();
    tmp.replace("__VERSION__", openscad_detailedversionnumber.c_str());
    this->aboutText->setHtml(tmp);
  }

public slots:
  void on_okPushButton_clicked() { accept(); }
};
