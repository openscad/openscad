#include "OpenCSGWarningDialog.h"
#include "Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *parent)
{
  setupUi(this);

	connect(this->showBox, SIGNAL(toggled(bool)),
					Preferences::inst()->openCSGWarningBox, SLOT(setChecked(bool)));
	connect(this->showBox, SIGNAL(toggled(bool)),
					Preferences::inst(), SLOT(openCSGWarningChanged(bool)));
}

void OpenCSGWarningDialog::setText(const QString &text)
{
  this->warningText->setPlainText(text);
}

