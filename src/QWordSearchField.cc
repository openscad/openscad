#include "QWordSearchField.h"
#include <QStyle>

QWordSearchField::QWordSearchField(QFrame *parent) : QLineEdit(parent) {
    findcount = 0;
    fieldLabel = new QLabel(this);
    fieldLabel->setTextFormat(Qt::PlainText);
    fieldLabel->setText(QString("00"));
    fieldLabel->setCursor(Qt::ArrowCursor);
    fieldLabel->setStyleSheet("QLabel { border: none; padding: 0px; }");
    fieldLabel->hide();
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateFieldLabel(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(fieldLabel->sizeHint().width() + frameWidth + 1));
    QSize minsize = minimumSizeHint();
    setMinimumSize(qMax(minsize.width(), fieldLabel->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(minsize.height(), fieldLabel->sizeHint().height() + frameWidth * 2 + 2));
}

void QWordSearchField::resizeEvent(QResizeEvent *) {
    resizeSearchField();
}

void QWordSearchField::resizeSearchField() {
    QSize size = fieldLabel->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    fieldLabel->move(rect().right() - frameWidth - size.width(), (rect().bottom() + 1 - size.height())/2);
}


void QWordSearchField::updateFieldLabel(const QString& text){
    fieldLabel->setVisible(!text.isEmpty());
    if (findcount > 0){
        fieldLabel->setText(QString::number(findcount));
    }else{
        fieldLabel->setText(QString(""));
    }
    resizeSearchField();
}
        
