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
    root.get_child(ParameterSet::parameterSetsKey).erase(setName);
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
	readParameterSet(this->jsonFile);
	connect(this->addButton, SIGNAL(clicked()), this, SLOT(onSetAdd()));
	connect(this->deleteButton, SIGNAL(clicked()), this, SLOT(onSetDelete()));
	this->comboBox->clear();
	setComboBoxForSet();
}

void ParameterWidget::writeFile(QString scadFile)
{
	writeParameterSet(scadFile.replace(".scad", ".json").toStdString());
}

void ParameterWidget::setComboBoxForSet()
{
	this->comboBox->addItem("No Set Selected", QVariant(QString::fromStdString("")));
	if (root.empty()) return;
	for (const auto &v : root.get_child(ParameterSet::parameterSetsKey)) {
		this->comboBox->addItem(QString::fromStdString(v.first),
														QVariant(QString::fromStdString(v.first)));
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
	for (group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++) {
		std::vector<std::string> gr;
		gr = it->second.parameterVector;
		for(unsigned int i=0;i < gr.size();i++) {
			AddParameterWidget(gr[i]);
		}
		GroupWidget *groupWidget = new GroupWidget(it->second.show, QString::fromStdString(it->first));
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
	for (group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++) {
		it->second.parameterVector.clear();
	}

	for (entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
		if (groupMap.find(it->second->groupName) == groupMap.end()) {
			groupInst enter;
			enter.parameterVector.push_back(it->first);
			enter.show = false;
			groupMap[it->second->groupName] = enter;
		}
		else {
			groupMap[it->second->groupName].parameterVector.push_back(it->first);
			
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
	if (root.empty()) return;
	
	std::string path = ParameterSet::parameterSetsKey + "." + setName;
	for (pt::ptree::value_type &v : root.get_child(path)) {
		entry_map_t::iterator entry = entries.find(v.first);
		if (entry != entries.end()) {
			if (entry->second->dvt == Value::STRING) {
				entry->second->value=ValuePtr(v.second.data());
			} else if (entry->second->dvt == Value::BOOL) {
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
		bool ok = true;
		QInputDialog *setDialog = new QInputDialog();
		setName = setDialog->getText(this, "Create new Set of parameter","Enter name of set name", QLineEdit::Normal, "", &ok).toStdString();
	}
	pt::ptree iroot;
	for (entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
		iroot.put(it->first,it->second->value->toString());
	}
	if (!setName.empty()) {
		root.get_child(ParameterSet::parameterSetsKey).erase(setName);
		root.add_child(ParameterSet::parameterSetsKey + "." + setName, iroot);
		if (this->comboBox->findText(QString::fromStdString(setName)) == -1) {
			this->comboBox->addItem(QString::fromStdString(setName),
															QVariant(QString::fromStdString(setName)));
			this->comboBox->setCurrentIndex(this->comboBox->findText(QString::fromStdString(setName)));
		}
	}
	writeParameterSet(this->jsonFile);
}
