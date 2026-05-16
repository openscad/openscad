#pragma once

#include <QDialog>
#include <QFile>
#include <QString>
#include <QWidget>
#include <string>

#include "gui/UIUtils.h"
#include "gui/qtgettext.h"
#include "ui_AboutDialog.h"
#include "version.h"

class AboutDialog : public QDialog, public Ui::AboutDialog
{
  Q_OBJECT;

public:
  AboutDialog(QWidget *)
  {
    setupUi(this);
    this->setWindowTitle(QString(_("About PythonSCAD")) + " " +
                         QString::fromStdString(std::string(openscad_shortversionnumber)));

    QString titleText = this->titleLabel->text();
    titleText.replace("__PYTHON_BRAND_COLOR__", UIUtils::pythonBrandColor);
    this->titleLabel->setText(titleText);

    QFile htmlFile(":/html/AboutDialog.html");
    if (htmlFile.open(QIODevice::ReadOnly)) {
      QString tmp = QString::fromUtf8(htmlFile.readAll());
      tmp.replace("__VERSION__", QString::fromStdString(std::string(openscad_detailedversionnumber)));
      tmp.replace("__PYTHON_BRAND_COLOR__", UIUtils::pythonBrandColor);
      this->aboutText->setHtml(tmp);
    }
  }

public slots:
  void on_okPushButton_clicked() { accept(); }
};
