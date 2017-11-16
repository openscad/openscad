#include "parametervirtualwidget.h"


ParameterVirtualWidget::ParameterVirtualWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

ParameterVirtualWidget::~ParameterVirtualWidget(){
	
}


void ParameterVirtualWidget::setName(QString name) {
	this->labelDescription->hide();
	name.replace(QRegExp("([_]+)"), " ");
	this->labelParameter->setText(name);
	this->labelInline->setText("");
}

void ParameterVirtualWidget::addInline(QString addTxt) {
	if(addTxt!=""){
		this->labelInline->setText(" - "+ addTxt);
	}
}

//calculate the required decimal precision.
//the result is stored in th member variable "decimalPrecision"
void ParameterVirtualWidget::setPrecision(double number){
	decimalPrecision = 0;
	long double diff, rn; //rn stands for real number
	unsigned long long intNumber, multi = 1;
	number = std::abs(number);
	while(1) {
		rn = (number * multi);
		intNumber = rn;  //the fractional part will be truncated here
		diff = rn - intNumber;
		if (diff <= 0.0 || decimalPrecision > 6) {
			break;
		}
		multi = multi * 10;
		decimalPrecision++;
	}
}

void ParameterVirtualWidget::setDescription(const QString& description) {
	if(!description.isEmpty()){
		this->labelDescription->show();
		this->labelDescription->setText(description);
	}
}
	
void ParameterVirtualWidget::resizeEvent(QResizeEvent * event){
	//bodge code to adjust the label height, when the label has to use multiple lines
	QLabel* label = this->labelDescription;
	
	int w=label->width();
	if(w!=LabelWidth){
		LabelWidth=w;
		label->setMinimumHeight(0);
		label->setMinimumHeight(label->heightForWidth(w));
	}
}
