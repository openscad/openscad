#pragma once

#include "qtgettext.h"
#include "ui_AboutDialog.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

class AboutDialog : public QDialog, public Ui::AboutDialog
{
	Q_OBJECT;
public:
	AboutDialog(QWidget *) {
		setupUi(this);
		this->setWindowTitle( QString("About OpenSCAD ") + QString(TOSTRING( OPENSCAD_VERSION)) );
		this->aboutText->setOpenExternalLinks(true);
		QUrl flattr_qurl(":icons/flattr.png" );
		this->aboutText->loadResource( QTextDocument::ImageResource, flattr_qurl );
		QString tmp = this->aboutText->toHtml();
		tmp.replace("__VERSION__",QString(TOSTRING(OPENSCAD_VERSION)));
		this->aboutText->setHtml(tmp);
	}
};
