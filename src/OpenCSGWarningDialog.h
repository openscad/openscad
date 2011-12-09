#ifndef OPENCSGWARNINGDIALOG_H_
#define OPENCSGWARNINGDIALOG_H_

#include "ui_OpenCSGWarningDialog.h"

class OpenCSGWarningDialog : public QDialog, public Ui::OpenCSGWarningDialog
{
	Q_OBJECT;
public:
	OpenCSGWarningDialog(QWidget *parent);

public slots:
	void setText(const QString &text);
};

#endif
