#include "groupwidget.h"

GroupWidget::GroupWidget(bool &show, const QString & title, QWidget *parent) : QWidget(parent)
{
	this->toggleButton.setText(title);
	this->toggleButton.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Maximum);
	this->toggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	this->toggleButton.setCheckable(true);

	this->show = &show;
	this->toggleButton.setChecked(show);

	// don't waste space
	this->mainLayout.setVerticalSpacing(0);
	this->mainLayout.setContentsMargins(0, 0, 0, 0);
	this->contentArea.setContentsMargins(0, 0, 0, 0);

	this->mainLayout.addWidget(&toggleButton, 0, 0);
	this->mainLayout.addWidget(&contentArea, 1, 0);
	setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Maximum);
	setLayout(&mainLayout);
	QObject::connect(&toggleButton, SIGNAL(toggled(bool)),this, SLOT(onclicked(bool)));
}


void GroupWidget::onclicked(const bool /*checked*/)
{
	this->toggleButton.setArrowType(this->toggleButton.isChecked() ? Qt::DownArrow : Qt::RightArrow);

	if (this->toggleButton.isChecked()) {
		*(this->show) = true; //update the show flag in the group map
		contentArea.show();
	} else {
		*(this->show) = false; //update the show flag in the group map
		contentArea.hide();
	}
}

void GroupWidget::setContentLayout(QLayout & contentLayout)
{
	delete this->contentArea.layout();

	this->contentArea.setLayout(&contentLayout);

	onclicked(this->toggleButton.isChecked());
}
