#include "gui/OpenCSGWarningDialog.h"
#include <QString>
#include <QWidget>
#include "gui/Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *)
{
  setupUi(this);
  #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  connect(this->showBox, &QCheckBox::toggled, Preferences::inst()->openCSGWarningBox, &QCheckBox::setChecked);
  connect(this->showBox, &QCheckBox::toggled, Preferences::inst(), &Preferences::on_openCSGWarningBox_toggled);
  #else
  connect(this->showBox, static_cast<void(QCheckBox::*)(bool)>(&QCheckBox::toggled), Preferences::inst()->openCSGWarningBox, &QCheckBox::setChecked);
  connect(this->showBox, static_cast<void(QCheckBox::*)(bool)>(&QCheckBox::toggled), Preferences::inst(), &Preferences::on_openCSGWarningBox_toggled);
  #endif
}

void OpenCSGWarningDialog::setText(const QString& text)
{
  this->warningText->setPlainText(text);
}

