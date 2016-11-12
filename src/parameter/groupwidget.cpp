#include "groupwidget.h"

#include <QPropertyAnimation>

GroupWidget::GroupWidget(bool &show, const QString & title, const int animationDuration, QWidget *parent) : QWidget(parent), animationDuration(animationDuration)
{
	toggleButton.setStyleSheet("QToolButton { border: none; }");
	toggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toggleButton.setText(title);
	toggleButton.setCheckable(true);
	
	headerLine.setFrameShape(QFrame::HLine);
	headerLine.setFrameShadow(QFrame::Sunken);
	headerLine.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	
	contentArea.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	// start out collapsed
	contentArea.setMaximumHeight(0);
	contentArea.setMinimumHeight(0);
	// let the entire widget grow and shrink with its content
	toggleAnimation.addAnimation(new QPropertyAnimation(this, "minimumHeight"));
	toggleAnimation.addAnimation(new QPropertyAnimation(this, "maximumHeight"));
	toggleAnimation.addAnimation(new QPropertyAnimation(&contentArea, "maximumHeight"));
	
	this->show = &show;
	toggleButton.setChecked(show);
	
	// don't waste space
	mainLayout.setVerticalSpacing(0);
	mainLayout.setContentsMargins(0, 0, 0, 0);
	int row = 0;
	mainLayout.addWidget(&toggleButton, row, 0, 1, 1, Qt::AlignLeft);
	mainLayout.addWidget(&headerLine, row++, 2, 1, 1);
	mainLayout.addWidget(&contentArea, row, 0, 1, 3);
	setLayout(&mainLayout);
	QObject::connect(&toggleButton, SIGNAL(toggled(bool)),this, SLOT(onclicked(bool)));
}


void GroupWidget::onclicked(const bool checked)
{
	toggleButton.setArrowType(toggleButton.isChecked() ? Qt::DownArrow : Qt::RightArrow);
	toggleAnimation.setDirection(toggleButton.isChecked() ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
	toggleAnimation.start();
	if (toggleButton.isChecked()) {
		*(this->show) = true;
	} else {
		*(this->show) = false;
	}
	this->animationDuration = 300;
}

void GroupWidget::setContentLayout(QLayout & contentLayout)
{
	delete contentArea.layout();
	contentArea.setLayout(&contentLayout);
	const int collapsedHeight = sizeHint().height() - contentArea.maximumHeight();
	int contentHeight = contentLayout.sizeHint().height();
	for (int i = 0; i < toggleAnimation.animationCount() - 1; ++i) {
		QPropertyAnimation * GroupWidgetAnimation = static_cast<QPropertyAnimation *>(toggleAnimation.animationAt(i));
		GroupWidgetAnimation->setDuration(animationDuration);
		GroupWidgetAnimation->setStartValue(collapsedHeight);
		GroupWidgetAnimation->setEndValue(collapsedHeight + contentHeight);
	}
	QPropertyAnimation * contentAnimation = static_cast<QPropertyAnimation *>(toggleAnimation.animationAt(toggleAnimation.animationCount() - 1));
	contentAnimation->setDuration(animationDuration);
	contentAnimation->setStartValue(0);
	contentAnimation->setEndValue(contentHeight);
	
	if (*(this->show)) {
		toggleButton.setArrowType(Qt::DownArrow);
		toggleAnimation.start();
	} else {
		toggleButton.setArrowType(Qt::RightArrow);
	}
}
