/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2014 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <QWidget>

#include "ParameterWidget.h"

#include "parameterspinbox.h"
#include "parametercombobox.h"
#include "parameterslider.h"
#include "parametercheckbox.h"
#include "parametertext.h"
#include "parametervector.h"
#include <QInputDialog>

#include "modcontext.h"
extern AssignmentList * parser(const char *text);

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
    descriptionShow=true;
	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
    connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
    connect(checkBoxDetailedDescription,SIGNAL(toggled(bool)),this,SLOT(onDescriptionShow()));
    connect(comboBox, SIGNAL(currentIndexChanged(int)),this,SLOT(onSetChanged(int)));
}

void ParameterWidget::onSetDelete(){
    if(root.empty()){
        return;
    }
    string setName=comboBox->itemData(this->comboBox->currentIndex()).toString().toStdString();
    root.get_child("SET").erase(setName);
    std::fstream myfile;
    myfile.open (this->jsonFile,ios::out);
    if(myfile.is_open()){
        pt::write_json(myfile,root);
        this->comboBox->clear();
        setComboBoxForSet();
    }
    myfile.close();
}

void ParameterWidget::onSetAdd(){

    if(root.empty()){
        pt::ptree setRoot;
        root.add_child("SET",setRoot);
    }
    updateParameterSet(comboBox->itemData(this->comboBox->currentIndex()).toString().toStdString());
}

void ParameterWidget::setFile(QString jsonFile){

    this->jsonFile=jsonFile.replace(".scad",".json").toStdString();
    getParameterSet(this->jsonFile);
    connect(addButton,SIGNAL(clicked()),this,SLOT(onSetAdd()));
    connect(deleteButton,SIGNAL(clicked()),this,SLOT(onSetDelete()));
    setComboBoxForSet();
}

ParameterWidget::~ParameterWidget()
{
}

void ParameterWidget::setComboBoxForSet(){

    this->comboBox->addItem("No Set Selected",
                QVariant(QString::fromStdString("")));
    if(root.empty()){
        return;
    }
    for(pt::ptree::value_type &v : root.get_child("SET")){
        this->comboBox->addItem(QString::fromStdString(v.first),
                      QVariant(QString::fromStdString(v.first)));
    }
}

void ParameterWidget::onSetChanged(int idx){

    const string v = comboBox->itemData(idx).toString().toUtf8().constData();
    applyParameterSet(v);
    emit previewRequested();

}

void ParameterWidget::onDescriptionShow()
{
    if(checkBoxDetailedDescription->isChecked()){
        descriptionShow=true;
    }else{
        descriptionShow=false;
    }
    emit previewRequested();
}

void ParameterWidget::onValueChanged()
{
	autoPreviewTimer.stop();
	if (checkBoxAutoPreview->isChecked()) {
		autoPreviewTimer.start();
	}
}

void ParameterWidget::onPreviewTimerElapsed()
{
	emit previewRequested();
}

void ParameterWidget::begin()
{
	QLayoutItem *child;
	while ((child = this->scrollAreaWidgetContents->layout()->takeAt(0)) != 0) {
		QWidget *w = child->widget();
        this->scrollAreaWidgetContents->layout()->removeWidget(w);

		delete w;
	}
	anyLayout = new QVBoxLayout();
}

void ParameterWidget::addEntry(ParameterVirtualWidget *entry)
{
	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	policy.setVerticalPolicy(QSizePolicy::Preferred);
	policy.setHorizontalStretch(0);
	policy.setVerticalStretch(0);
    entry->setSizePolicy(policy);
    anyLayout->addWidget(entry);

}

void ParameterWidget::end()
{
	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	policy.setVerticalPolicy(QSizePolicy::Preferred);
	policy.setHorizontalStretch(0);
	policy.setVerticalStretch(1);
	QLabel *label = new QLabel("");
	label->setSizePolicy(policy);
	this->scrollAreaWidgetContents->layout()->addWidget(label);
}

void ParameterWidget::connectWidget()
{
    for(entry_map_t::iterator it = entries.begin(); it != entries.end();) {
        if(!(*it).second->set){
            it=entries.erase(it);
        }else{
            it++;
        }
    }

     for(group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++) {
         it->second.parameterVector.clear();
     }

    for(entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
            if(groupMap.find(it->second->groupName) == groupMap.end()){
                groupInst enter;
                enter.parameterVector.push_back(it->first);
                enter.show=false;
                groupMap[it->second->groupName]=enter;
            }
            else{
                groupMap[it->second->groupName].parameterVector.push_back(it->first);

            }
     }

vector<string> global;
if(groupMap.find("Global")!=groupMap.end()){
    global=groupMap["Global"].parameterVector;
    groupMap["Global"].parameterVector.clear();
}

 for(group_map::iterator it = groupMap.begin(); it != groupMap.end(); ) {
        vector<string> gr;
        gr=it->second.parameterVector; 
        if(gr.empty()|| it->first=="Hidden"){
            it=groupMap.erase(it);
        }
        else{
   
            it->second.parameterVector.insert( it->second.parameterVector.end(), global.begin(), global.end() );
            it++;
    }
}

    begin();
    for(group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++) {
        vector<string> gr;
        gr=it->second.parameterVector;
       
        for(int i=0;i <gr.size();i++){
            ParameterVirtualWidget *entry ;
            switch (entries[gr[i]]->target) {
                case COMBOBOX:{
                    entry = new ParameterComboBox(entries[gr[i]],descriptionShow);
                    break;
                }
                case SLIDER:{
                    entry = new ParameterSlider(entries[gr[i]],descriptionShow);
                    break;
                }
                case CHECKBOX:{
                    entry = new ParameterCheckBox(entries[gr[i]],descriptionShow);
                    break;
                }
                case TEXT:{
                    entry = new ParameterText(entries[gr[i]],descriptionShow);
                    break;
                }
                case NUMBER:{
                    entry = new ParameterSpinBox(entries[gr[i]],descriptionShow);
                    break;
                }
                case VECTOR:{
                    entry = new ParameterVector(entries[gr[i]],descriptionShow);
                    break;
                }
            }
            if(entries[gr[i]]->target!=UNDEFINED){
                connect(entry, SIGNAL(changed()), this, SLOT(onValueChanged()));
                addEntry(entry);
                
            }
        }
        GroupWidget *groupWidget =new GroupWidget(it->second.show ,QString::fromStdString(it->first));
        groupWidget->setContentLayout(*anyLayout);
        this->scrollAreaWidgetContents->layout()->addWidget(groupWidget);
	    anyLayout = new QVBoxLayout();
    }
    end();
}

void ParameterWidget::applyParameterSet(string setName){

    if(root.empty()){
        return;
    }
    string path="SET."+setName;
    Parameterset::iterator set=parameterSet.find(setName);
    for(pt::ptree::value_type &v : root.get_child(path)){
        entry_map_t::iterator entry =entries.find(v.first);
            if(entry!=entries.end()){
                if(entry->second->dvt == Value::NUMBER){
                    entry->second->value=ValuePtr(v.second.get_value<double>());
                }else if(entry->second->dvt== Value::BOOL){
                    entry->second->value=ValuePtr(v.second.get_value<bool>());
                }else if(entry->second->dvt== Value::VECTOR){
                    ModuleContext ctx;
                    AssignmentList *assignmentList;
                    assignmentList=parser(v.second.data().c_str());
                    Assignment *assignment;
                    for(int i=0; i<assignmentList->size(); i++) {
                        assignment=assignmentList[i].data();
                    }
                    entry->second->value=assignment->expr.get()->evaluate(&ctx);
                }else if(entry->second->dvt== Value::STRING){
                 entry->second->value=ValuePtr(v.second.data());
                }
            }
    }
}

void ParameterWidget::updateParameterSet(string setName){
    std::fstream myfile;
    myfile.open (this->jsonFile,ios::out);
    if(myfile.is_open()){
        if(setName==""){
            QInputDialog *a=new QInputDialog();
            a->setLabelText("Name of new set");
            setName=a->getText(NULL,"Create new Set of parameter","Enter name of set name").toStdString();
            if(setName.empty()){
                return;
             }
        }
        pt::ptree iroot;
        for(entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
            iroot.put(it->first,it->second->value->toString());
        }
        root.get_child("SET").erase(setName);
        root.add_child("SET."+setName,iroot);
        pt::write_json(myfile,root);
        if(this->comboBox->findText(QString::fromStdString(setName))==-1){
            this->comboBox->addItem(QString::fromStdString(setName),
                      QVariant(QString::fromStdString(setName)));
            this->comboBox->setCurrentIndex(this->comboBox->findText(QString::fromStdString(setName)));
        }
    }
    myfile.close();

}
