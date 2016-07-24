#include "parametertext.h"

#include "modcontext.h"
extern AssignmentList * parser(const char *text);

ParameterText::ParameterText(ParameterObject *parameterobject, bool showDescription)
{
    object=parameterobject;
    setName(QString::fromStdString(object->name));
    setValue();
    connect(lineEdit,SIGNAL(editingFinished()),this,SLOT(on_Changed()));
    if(showDescription==true){
        setDescription(object->description);
    }
    else{
        lineEdit->setToolTip(object->description);
    }
}

void ParameterText::on_Changed()
{
    if(object->dvt == Value::STRING){
        object->value = ValuePtr(lineEdit->text().toStdString());
    }
    else{
        ModuleContext ctx;
        AssignmentList *assignmentList;
        assignmentList=parser(lineEdit->text().toStdString().c_str());
        if(assignmentList==NULL){
            return ;
        }
        for(unsigned int i=0; i<assignmentList->size(); i++) {
            ValuePtr newValue=assignmentList[i].data()->expr.get()->evaluate(&ctx);
            if(object->dvt==newValue->type()){
                object->value=newValue;
            }
        }
    }
    emit changed();
}

void ParameterText::setValue()
{
    this->stackedWidget->setCurrentWidget(this->pageText);
    this->lineEdit->setText(QString::fromStdString(object->value->toString()));
    if(object->values->toDouble()>0){
        this->lineEdit->setMaxLength(object->values->toDouble());
    }
}
