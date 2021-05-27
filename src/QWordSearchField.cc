#include "QWordSearchField.h"
#include <QStyle>

QWordSearchField::QWordSearchField(QFrame *parent) : QLineEdit(parent)
{
	findcount = 0;
	fieldLabel = new QLabel(this);
	fieldLabel->setTextFormat(Qt::PlainText);
	fieldLabel->setText(QString("00"));
	fieldLabel->setCursor(Qt::ArrowCursor);
	fieldLabel->setStyleSheet("QLabel { border: none; padding: 0px; }");
	fieldLabel->hide();
	connect(this, SIGNAL(findCountChanged()), this, SLOT(updateFieldLabel()));
	auto frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(fieldLabel->sizeHint().width() + frameWidth + 1));
	auto minsize = minimumSizeHint();
	setMinimumSize(qMax(minsize.width(), fieldLabel->sizeHint().height() + frameWidth * 2 + 2),
								 qMax(minsize.height(), fieldLabel->sizeHint().height() + frameWidth * 2 + 2));
	fieldLabel->setAlignment(Qt::AlignRight);
}

void QWordSearchField::resizeEvent(QResizeEvent *)
{
	resizeSearchField();
}

void QWordSearchField::resizeSearchField()
{
	auto size = fieldLabel->sizeHint();
	auto frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	fieldLabel->move(rect().right() - frameWidth - size.width(), (rect().bottom() + 1 - size.height())/2);
}


void QWordSearchField::updateFieldLabel()
{
	if (findcount > 0) {
		fieldLabel->setText(QString::number(findcount));
		//Fixes issue #2962 : Due to that fieldLabel->setText above does not seem to change the size of the field correct (seems to always be to short field to accommodate all digits)
		//when the field changes many times during several searches, we need to work around that by setting minimum size.
		fieldLabel->setMinimumSize(fieldLabel->minimumSizeHint());
		fieldLabel->setVisible(true);
	} else {
		fieldLabel->setText(QString(""));
		fieldLabel->setVisible(false);
	}
	resizeSearchField();
}
    
void QWordSearchField::setFindCount(int value)
{
	if (value != findcount) {
		findcount = value;
		emit findCountChanged();
	}
}
