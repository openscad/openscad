#include "OpenCSGWarningDialog.h"
#include "Preferences.h"

OpenCSGWarningDialog::OpenCSGWarningDialog(QWidget *)
{
	setupUi(this);

	connect(this->showBox, SIGNAL(toggled(bool)),
					Preferences::inst()->openCSGWarningBox, SLOT(setChecked(bool)));
	connect(this->showBox, SIGNAL(toggled(bool)),
					Preferences::inst(), SLOT(on_openCSGWarningBox_toggled(bool)));

	connect(this->enableOpenCSGBox, SIGNAL(toggled(bool)),
					Preferences::inst()->enableOpenCSGBox, SLOT(setChecked(bool)));
	connect(this->enableOpenCSGBox, SIGNAL(toggled(bool)),
					Preferences::inst(), SLOT(on_enableOpenCSGBox_toggled(bool)));
}

void OpenCSGWarningDialog::setText(const QString &text)
{
	this->warningText->setPlainText(text);
}

