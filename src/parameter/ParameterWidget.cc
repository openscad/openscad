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

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>

namespace pt = boost::property_tree;

#include <QInputDialog>
#include <QMessageBox>
#include <fstream>

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
	this->setEnabled(false);

	descriptionLoD = 0;
	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
	connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
	connect(comboBoxDetails,SIGNAL(currentIndexChanged(int)), this,SLOT(onDescriptionLoDChanged()));
	connect(comboBoxPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));
	connect(reset, SIGNAL(clicked()), this, SLOT(resetParameter()));

	this->extractor = new ParameterExtractor();
	this->setMgr = new ParameterSet();
}

void ParameterWidget::resetParameter()
{
	this->entries.clear(); //clearing the entries forces a reset
	emit previewRequested();
}

ParameterWidget::~ParameterWidget()
{
}

//deletes the currently selected/active parameter set
void ParameterWidget::onSetDelete()
{
	if (setMgr->isEmpty()) return;
	std::string setName=comboBoxPreset->itemData(this->comboBoxPreset->currentIndex()).toString().toStdString();
	boost::optional<pt::ptree &> sets = setMgr->parameterSets();
	if (sets.is_initialized()) {
		sets.get().erase(pt::ptree::key_type(setName));
	}
	writeParameterSets();
	this->comboBoxPreset->clear();
	setComboBoxPresetForSet();
}

//adds a new parameter set
void ParameterWidget::onSetAdd()
{
	if (setMgr->isEmpty()) {
		pt::ptree setRoot;
		setMgr->addChild(ParameterSet::parameterSetsKey, setRoot);
	}
	updateParameterSet("");
}

void ParameterWidget::onSetSaveButton()
{
	if (setMgr->isEmpty()) {
		pt::ptree setRoot;
		setMgr->addChild(ParameterSet::parameterSetsKey, setRoot);
	}
	updateParameterSet(comboBoxPreset->itemData(this->comboBoxPreset->currentIndex()).toString().toStdString());
}

void ParameterWidget::setFile(QString scadFile){
	this->unreadableFileExists = false; // we can not know it, but we do not want to report a problem when there is none
	boost::filesystem::path p = scadFile.toStdString();
	this->jsonFile = p.replace_extension(".json").string();
}

void ParameterWidget::readFile(QString scadFile)
{
	setFile(scadFile);
	bool exists = boost::filesystem::exists(this->jsonFile);
	bool writeable = false;
	bool readable = false;

	if(exists){
		readable = setMgr->readParameterSet(this->jsonFile);

		//check whether file is writeable or not
		if (std::fstream(this->jsonFile, std::ios::app)) writeable = true;
	}

	if(writeable || !exists){
		connect(this->addButton, SIGNAL(clicked()), this, SLOT(onSetAdd()));
		this->addButton->setToolTip(_("add new preset"));
		connect(this->deleteButton, SIGNAL(clicked()), this, SLOT(onSetDelete()));
		this->deleteButton->setToolTip(_("remove current preset"));
		connect(this->presetSaveButton, SIGNAL(clicked()), this, SLOT(onSetSaveButton()));
		this->presetSaveButton->setToolTip(_("save current preset"));
	}else{
		this->addButton->setToolTip(_("JSON file read only"));
		this->deleteButton->setToolTip(_("JSON file read only"));
		this->presetSaveButton->setToolTip(_("JSON file read only"));
	}
	this->addButton->setEnabled(writeable || !exists);
	this->deleteButton->setEnabled(writeable || !exists);
	this->presetSaveButton->setEnabled(writeable || !exists);

	this->unreadableFileExists = (!readable) && exists;

	disconnect(comboBoxPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));
	this->comboBoxPreset->clear();
	setComboBoxPresetForSet();
	connect(comboBoxPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));

}

void ParameterWidget::writeFileIfNotEmpty(QString scadFile)
{
	setFile(scadFile);
	if (!setMgr->isEmpty()){
		writeParameterSets();
	}
}

void ParameterWidget::setParameters(const FileModule* module,bool rebuildParameterWidget)
{
	this->extractor->setParameters(module,this->entries,this->groupConditions,ParameterPos,rebuildParameterWidget);
	if(rebuildParameterWidget){
		connectWidget();
	}else{
		updateWidget();
	}
}

void ParameterWidget::applyParameters(FileModule *fileModule)
{
	this->extractor->applyParameters(fileModule,entries);
}

void ParameterWidget::setComboBoxPresetForSet()
{
	this->comboBoxPreset->addItem(_("no preset selected"), QVariant(QString::fromStdString("")));
	if (setMgr->isEmpty()) return;
	for (const auto &name : setMgr->getParameterNames()) {
		const QString n = QString::fromStdString(name);
		this->comboBoxPreset->addItem(n, QVariant(n));
	}
}

void ParameterWidget::onSetChanged(int idx)
{
	const std::string v = comboBoxPreset->itemData(idx).toString().toUtf8().constData();
	applyParameterSet(v);
	emit previewRequested(false);
}

void ParameterWidget::onDescriptionLoDChanged()
{
	descriptionLoD =comboBoxDetails->currentIndex();
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
	emit previewRequested(false);
}

void ParameterWidget::cleanScrollArea()
{
	auto layout = this->scrollAreaWidgetContents->layout();
	layout->setAlignment(Qt::AlignTop);
	QLayoutItem *child;
	while ((child = layout->takeAt(0)) != nullptr) {
		QWidget *w = child->widget();
		layout->removeWidget(w);
		delete w;
	}
}

void ParameterWidget::connectWidget()
{
	this->setEnabled(true);

	this->anyfocused = false;

	rebuildGroupMap();
	
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
	cleanScrollArea();

	for (const auto &groupName : groupPos) {
		if(groupMap.find(groupName)!=groupMap.end()){
			QVBoxLayout* anyLayout = new QVBoxLayout();
			anyLayout->setSpacing(0);
			anyLayout->setContentsMargins(0,0,0,0);

			std::vector<std::string> gr;
			gr = groupMap[groupName].parameterVector;
			for(unsigned int i=0;i < gr.size();i++) {
				ParameterVirtualWidget * entry = CreateParameterWidget(gr[i]);
				if(entry){
					anyLayout->addWidget(entry);
				}
			}

			GroupWidget *groupWidget = new GroupWidget(groupMap[groupName].show, QString::fromStdString(groupName));
			groupWidget->setContentLayout(*anyLayout);
			groupWidget->setVisible(!groupMap[groupName].hide);
			this->scrollAreaWidgetContents->layout()->addWidget(groupWidget);
		}
	}
	if (anyfocused != 0){
		entryToFocus->setParameterFocus();
	}
}

void ParameterWidget::updateWidget()
{
	QList<ParameterVirtualWidget *> parameterWidgets = this->findChildren<ParameterVirtualWidget *>();
	foreach(ParameterVirtualWidget* widget, parameterWidgets)
		widget->setValue();
}

void ParameterWidget::rebuildGroupMap(){
	for (entry_map_t::iterator it = entries.begin(); it != entries.end();) {
		if (!(*it).second->set) {
			it = entries.erase(it);
		} else {
			it++;
		}
	}
	for (auto &elem : groupMap) {
		elem.second.parameterVector.clear();
		elem.second.inList=false;
	}

	groupPos.clear();
	for (unsigned int it=0; it<ParameterPos.size(); it++) {
		std::string groupName=entries[ParameterPos[it]]->groupName;
		if (groupMap.find(groupName) == groupMap.end()) {
			groupPos.push_back(groupName);
			groupInst enter;
			enter.parameterVector.push_back(ParameterPos[it]);
			enter.show = false;
			enter.inList=true;
			enter.hide=false;
			groupMap[groupName] = enter;
		}else {
			if(groupMap[groupName].inList == false){
				groupMap[groupName].inList=true;
				groupPos.push_back(groupName);
			}
			groupMap[groupName].parameterVector.push_back(ParameterPos[it]);
		}
	}
}

ParameterVirtualWidget* ParameterWidget::CreateParameterWidget(std::string parameterName)
{
	ParameterVirtualWidget *entry = nullptr;
	switch(entries[parameterName]->target) {
		case ParameterObject::COMBOBOX:{
			entry = new ParameterComboBox(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::SLIDER:{
			entry = new ParameterSlider(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::CHECKBOX:{
			entry = new ParameterCheckBox(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::TEXT:{
			entry = new ParameterText(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::NUMBER:{
			entry = new ParameterSpinBox(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::VECTOR:{
			entry = new ParameterVector(entries[parameterName], descriptionLoD);
			break;
		}
		case ParameterObject::UNDEFINED:{
			break;
		}
    }
    if (entry) {
		connect(entry, SIGNAL(changed()), this, SLOT(onValueChanged()));
		if (entries[parameterName]->focus){
			entryToFocus = entry;
			anyfocused = true;
		}
	}
	return entry;
}

void ParameterWidget::applyParameterSet(std::string setName)
{
	boost::optional<pt::ptree &> set = setMgr->getParameterSet(setName);
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
			_("Create new set of parameter"), _("Enter name of the parameter set"),
			QLineEdit::Normal, "", &ok);

		if (ok) {
			setName = result.trimmed().toStdString();
		}
	}

	if (!setName.empty()) {
		pt::ptree iroot;
		for (const auto &entry : entries) {
			if (entry.second->groupName != "Hidden") {
				const auto &VariableName = entry.first;
				const auto &VariableValue = entry.second->value->toString();
				iroot.put(VariableName, VariableValue);
			}
		}
		setMgr->addParameterSet(setName, iroot);
		const QString s(QString::fromStdString(setName));
		if (this->comboBoxPreset->findText(s) == -1) {
			this->comboBoxPreset->addItem(s, QVariant(s));
			this->comboBoxPreset->setCurrentIndex(this->comboBoxPreset->findText(s));
		}
		writeParameterSets();
	}
}

void ParameterWidget::writeParameterSets()
{
	if(this->unreadableFileExists){
		QMessageBox msgBox;
		msgBox.setText(_("Saving presets"));
		msgBox.setInformativeText(QString(_("%1 was found, but was unreadble. Do you want to overwrite %1?")).arg(QString::fromStdString(this->jsonFile)));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);

		if (msgBox.exec() == QMessageBox::Cancel) {
			return;
		}
	}
	setMgr->writeParameterSet(this->jsonFile);
}

void ParameterWidget::updateCondition(Context& top_ctx){
	for (const auto &groupCondition : groupConditions) {
		std::string groupName = groupCondition.first; 
		std::string condition = groupCondition.second;

		if(condition!=""){
			bool state = top_ctx.lookup_variable(condition,true)->toBool();
			groupMap[groupName].hide=!state;
			groupMap[groupName].show=state;
		}else{
			groupMap[groupName].hide=false;
		}
	}
}

void ParameterWidget::applyCondition(){
	for (const auto &groupName : groupPos) {
		if(groupMap.find(groupName)!=groupMap.end()){
			GroupWidget* groupWidget = this->findChild<GroupWidget*>(QString(("GroupWidget"+groupName).c_str()));
			groupWidget->setVisible(!groupMap[groupName].hide);
		}
	}
}
