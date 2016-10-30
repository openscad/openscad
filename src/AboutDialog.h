#pragma once

#include "openscad.h"
#include "qtgettext.h"
#include "ui_AboutDialog.h"
#include "PlatformUtils.h"

class AboutDialog : public QDialog, public Ui::AboutDialog
{
	Q_OBJECT;
public:
	AboutDialog(QWidget *) {
		setupUi(this);
		this->setWindowTitle( QString(_("About OpenSCAD")) + " " + PlatformUtils::shortversion().c_str());
		QString tmp = this->aboutText->toHtml();
		tmp.replace("__VERSION__", PlatformUtils::detailedversion().c_str());
		this->aboutText->setHtml(tmp);
	}

public slots:
        void on_okPushButton_clicked() { accept(); }
};
