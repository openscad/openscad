#include "groupwidget.h"

GroupWidget::GroupWidget(bool &show, const QString & title, QWidget *parent) : QWidget(parent)
{
	toggleButton.setText(title);
	toggleButton.setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Maximum);
	toggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toggleButton.setCheckable(true);

	this->show = &show;
	toggleButton.setChecked(show);

	// don't waste space
	mainLayout.setVerticalSpacing(0);
	mainLayout.setContentsMargins(0, 0, 0, 0);
    contentArea.setContentsMargins(0, 0, 0, 0);

	mainLayout.addWidget(&toggleButton, 0, 0, nullptr);
	mainLayout.addWidget(&contentArea, 1, 0, nullptr);
	setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Maximum);
	setLayout(&mainLayout);
	QObject::connect(&toggleButton, SIGNAL(toggled(bool)),this, SLOT(onclicked(bool)));
}


void GroupWidget::onclicked(const bool /*checked*/)
{
	toggleButton.setArrowType(toggleButton.isChecked() ? Qt::DownArrow : Qt::RightArrow);

	if (toggleButton.isChecked()) {
		*(this->show) = true;
		contentArea.show();
	} else {
		*(this->show) = false;
		contentArea.hide();
	}
}

void GroupWidget::setContentLayout(QLayout & contentLayout)
{
	delete contentArea.layout();

	contentArea.setLayout(&contentLayout);

	onclicked(toggleButton.isChecked());
}
