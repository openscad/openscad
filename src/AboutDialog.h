#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

#include "ui_AboutDialog.h"

class AboutDialog : public QDialog, public Ui::AboutDialog
{
	Q_OBJECT;
public:
	AboutDialog(QWidget *) {
		setupUi(this);
		this->aboutText->setOpenExternalLinks(true);
		QUrl flattr_qurl(":icons/flattr.png" );
		this->aboutText->loadResource( QTextDocument::ImageResource, flattr_qurl );
	}
};

#endif
