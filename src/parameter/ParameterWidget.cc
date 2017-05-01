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

#include "modcontext.h"
#include "comment.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;

#include <QInputDialog>
#include <fstream>

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	descriptionShow = true;
	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
	connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
	connect(checkBoxDetailedDescription,SIGNAL(toggled(bool)), this,SLOT(onDescriptionShow()));
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));
	connect(reset, SIGNAL(clicked()), this, SLOT(resetParameter()));
}

void ParameterWidget::resetParameter()
{
	this->resetPara = true;
	emit previewRequested();
}

ParameterWidget::~ParameterWidget()
{
}

void ParameterWidget::onSetDelete()
{
	if (root.empty()) return;
	std::string setName=comboBox->itemData(this->comboBox->currentIndex()).toString().toStdString();
	root.get_child(ParameterSet::parameterSetsKey).erase(pt::ptree::key_type(setName));
	writeParameterSet(this->jsonFile);
	this->comboBox->clear();
	setComboBoxForSet();
}

void ParameterWidget::onSetAdd()
{
	if (root.empty()) {
		pt::ptree setRoot;
		root.add_child(ParameterSet::parameterSetsKey, setRoot);
	}
	updateParameterSet(comboBox->itemData(this->comboBox->currentIndex()).toString().toStdString());
}

void ParameterWidget::readFile(QString scadFile)
{
	this->jsonFile = scadFile.replace(".scad", ".json").toStdString();
	bool readonly=readParameterSet(this->jsonFile);
	if(readonly){
		connect(this->addButton, SIGNAL(clicked()), this, SLOT(onSetAdd()));
		connect(this->deleteButton, SIGNAL(clicked()), this, SLOT(onSetDelete()));
	}
	else{
		this->addButton->setDisabled(true);
		this->addButton->setToolTip("JSON file read only");
		this->deleteButton->setDisabled(true);
		this->deleteButton->setToolTip("JSON file read only");
	}
	disconnect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));
	this->comboBox->clear();
	setComboBoxForSet();
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));

}

void ParameterWidget::writeFile(QString scadFile)
{
	writeParameterSet(scadFile.replace(".scad", ".json").toStdString());
}

void ParameterWidget::setComboBoxForSet()
{
	this->comboBox->addItem("No Set Selected", QVariant(QString::fromStdString("")));
	if (root.empty()) return;
	for (const auto &name : getParameterNames()) {
		const QString n = QString::fromStdString(name);
		this->comboBox->addItem(n, QVariant(n));
	}
}

void ParameterWidget::onSetChanged(int idx)
{
	const std::string v = comboBox->itemData(idx).toString().toUtf8().constData();
	applyParameterSet(v);
	emit previewRequested();
}

void ParameterWidget::onDescriptionShow()
{
	if (checkBoxDetailedDescription->isChecked()) {
		descriptionShow = true;
	} else {
		descriptionShow = false;
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
	anyfocused = false;
	// clear previous entries in groupMap and entries
	clear();
	
	std::vector<std::string> global;
	if (groupMap.find("Global") != groupMap.end()) {
		global = groupMap["Global"].parameterVector;
		groupMap["Global"].parameterVector.clear();
	}
	
	for (group_map::iterator it = groupMap.begin(); it != groupMap.end(); ) {
		std::vector<std::string> gr;
		gr = it->second.parameterVector;
		if (gr.empty() || it->first == "Hidden") {
			it = groupMap.erase(it);
		}
		else {
			it->second.parameterVector.insert(it->second.parameterVector.end(), global.begin(), global.end());
			it++;
		}
	}
	begin();
	for (std::vector<std::string>::iterator it = groupPos.begin(); it != groupPos.end(); it++) {
		if(groupMap.find(*it)==groupMap.end())
			continue;
		std::vector<std::string> gr;
		gr = groupMap[*it].parameterVector;
		for(unsigned int i=0;i < gr.size();i++) {
			AddParameterWidget(gr[i]);
		}
		GroupWidget *groupWidget = new GroupWidget(groupMap[*it].show, QString::fromStdString(*it));
		groupWidget->setContentLayout(*anyLayout);
		this->scrollAreaWidgetContents->layout()->addWidget(groupWidget);
		anyLayout = new QVBoxLayout();
	}
	end();
	if (anyfocused != 0){
		entryToFocus->setParameterFocus();
	}
}

void ParameterWidget::clear(){
	for (entry_map_t::iterator it = entries.begin(); it != entries.end();) {
		if (!(*it).second->set) {
			it = entries.erase(it);
		} else {
			it++;
		}
	}
	for (group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++){
		it->second.parameterVector.clear();
		it->second.inList=false;
	}

	groupPos.clear();
	for (int it=0; it<ParameterPos.size(); it++) {
		std::string groupName=entries[ParameterPos[it]]->groupName;
		if (groupMap.find(groupName) == groupMap.end()) {
			groupPos.push_back(groupName);
			groupInst enter;
			enter.parameterVector.push_back(ParameterPos[it]);
			enter.show = false;
			enter.inList=true;
			groupMap[groupName] = enter;
		}
		else {
			if(groupMap[groupName].inList == false){
				groupMap[groupName].inList=true;
				groupPos.push_back(groupName);
			}
			groupMap[groupName].parameterVector.push_back(ParameterPos[it]);
		}
	}
}

void ParameterWidget::AddParameterWidget(std::string parameterName)
{
    ParameterVirtualWidget *entry ;
    switch(entries[parameterName]->target) {
		case ParameterObject::COMBOBOX:{
			entry = new ParameterComboBox(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::SLIDER:{
			entry = new ParameterSlider(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::CHECKBOX:{
			entry = new ParameterCheckBox(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::TEXT:{
			entry = new ParameterText(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::NUMBER:{
			entry = new ParameterSpinBox(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::VECTOR:{
			entry = new ParameterVector(entries[parameterName], descriptionShow);
			break;
		}
		case ParameterObject::UNDEFINED:{
			break;
		}
    }
    if (entries[parameterName]->target != ParameterObject::UNDEFINED) {
			connect(entry, SIGNAL(changed()), this, SLOT(onValueChanged()));
			addEntry(entry);
			if (entries[parameterName]->focus){
				entryToFocus = entry;
				anyfocused = true;
			}
    }
}

void ParameterWidget::applyParameterSet(std::string setName)
{
	boost::optional<pt::ptree &> set = getParameterSet(setName);
	if (!set.is_initialized()) {
		return;
	}

	for (pt::ptree::value_type &v : set.get()) {
		entry_map_t::iterator entry = entries.find(v.first);
		if (entry != entries.end()) {
			if (entry->second->dvt == Value::ValueType::STRING) {
				entry->second->value=ValuePtr(v.second.data());
			} else if (entry->second->dvt == Value::ValueType::BOOL) {
				entry->second->value = ValuePtr(v.second.get_value<bool>());
			} else {
				shared_ptr<Expression> params = CommentParser::parser(v.second.data().c_str());
				if (!params) continue;
				
				ModuleContext ctx;
				ValuePtr newValue = params->evaluate(&ctx);
				if (entry->second->dvt == newValue->type()) {
					entry->second->value = newValue;
				}
			}
		}
	}
}

void ParameterWidget::updateParameterSet(std::string setName)
{
	if (setName == "") {
		QInputDialog *setDialog = new QInputDialog();

		bool ok = true;
		const QString result = setDialog->getText(this,
			"Create new set of parameter", "Enter name of the parameter set",
			QLineEdit::Normal, "", &ok);

		if (ok) {
			setName = result.trimmed().toStdString();
		}
	}

	if (!setName.empty()) {
		pt::ptree iroot;
		for (entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
			iroot.put(it->first,it->second->value->toString());
		}
		addParameterSet(setName, iroot);
		const QString s(QString::fromStdString(setName));
		if (this->comboBox->findText(s) == -1) {
			this->comboBox->addItem(s, QVariant(s));
			this->comboBox->setCurrentIndex(this->comboBox->findText(s));
		}
		writeParameterSet(this->jsonFile);
	}
}
