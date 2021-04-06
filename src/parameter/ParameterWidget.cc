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

	descriptionLoD = DescLoD::ShowDetails;
	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
	connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
	connect(comboBoxDetails,SIGNAL(currentIndexChanged(int)), this,SLOT(onDescriptionLoDChanged()));
	connect(comboBoxPreset, SIGNAL(currentIndexChanged(int)), this, SLOT(onSetChanged(int)));
	connect(comboBoxPreset->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onSetNameChanged()));
	connect(reset, SIGNAL(clicked()), this, SLOT(resetParameter()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(onSetAdd()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(onSetDelete()));
	connect(presetSaveButton, SIGNAL(clicked()), this, SLOT(onSetSaveButton()));

	comboBoxPreset->setInsertPolicy(QComboBox::InsertAtCurrent);

	this->extractor = new ParameterExtractor();
	this->setMgr = new ParameterSet();
	this->valueChanged=false;
}

//resets all parameters to the currently selected parameter set
void ParameterWidget::resetParameter()
{
	if(this->valueChanged){
		QMessageBox msgBox;
		msgBox.setWindowTitle(_("changes on current preset not saved"));
		msgBox.setText(
			QString(_("The changes on the current preset %1 are not saved yet. Do you really want to reset this preset and lose your changes?"))
			.arg(comboBoxPreset->itemData(this->comboBoxPreset->currentIndex()).toString()));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::Cancel) {
			return;
		}
	}
	this->valueChanged=false;

	int currPreset = this->comboBoxPreset->currentIndex();

	removeChangeIndicator();

	defaultParameter();
	if(comboBoxPreset->currentIndex() != 0){ //0 is "design default values"
		const std::string v = comboBoxPreset->itemData(currPreset).toString().toUtf8().constData();
		applyParameterSet(v);
	}
	emit previewRequested();
}

ParameterWidget::~ParameterWidget()
{
}

//deletes the currently selected/active parameter set
void ParameterWidget::onSetDelete()
{
	if (this->setMgr->isEmpty()) return;
	std::string setName=comboBoxPreset->itemData(this->comboBoxPreset->currentIndex()).toString().toStdString();
	boost::optional<pt::ptree &> sets = this->setMgr->parameterSets();
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
	updateParameterSet("",true);
}

void ParameterWidget::onSetSaveButton()
{
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

	this->setMgr->clear();
	if (exists) {
		readable = this->setMgr->readParameterSet(this->jsonFile);
		//check whether file is writeable or not
		if (std::fstream(this->jsonFile, std::ios::app)) writeable = true;
	}

	if(writeable || !exists){
		this->addButton->setToolTip(_("Add new preset"));
		this->deleteButton->setToolTip(_("Remove current preset"));
		this->presetSaveButton->setToolTip(_("Save current preset"));
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

//Write the json file if the parameter sets are not empty.
//This prevents creating unnecessary json files.
//This method also updates the UI state (change indicator, file name, ...)
void ParameterWidget::writeFileIfNotEmpty(QString scadFile)
{
	setFile(scadFile);
	if (!this->setMgr->isEmpty()){
		writeParameterSets();
	}
}

//Write the json file without side effects (e.g. change indicator, file name)
//This is e.g. useful when saving hidden back up files.
void ParameterWidget::writeBackupFile(QString scadFile)
{
	if (!this->setMgr->isEmpty()){
		boost::filesystem::path p = scadFile.toStdString();
		auto jsonFile = p.replace_extension(".json").string();
		this->setMgr->writeParameterSet(jsonFile);
	}
}

void ParameterWidget::setParameters(const FileModule* module,bool rebuildParameterWidget)
{
	this->extractor->setParameters(module,this->entries,ParameterPos,rebuildParameterWidget);
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
	this->lastComboboxIndex = -1;
	this->comboBoxPreset->addItem(_("design default values"), QVariant(QString::fromStdString(_("design default values"))));
	if (this->setMgr->isEmpty()) return;
	for (const auto &name : this->setMgr->getParameterNames()) {
		const QString n = QString::fromStdString(name);
		this->comboBoxPreset->addItem(n, QVariant(n));
	}
}

//set selection
void ParameterWidget::onSetChanged(int idx)
{
	if(this->lastComboboxIndex == idx) return; //nothing todo

	//if necessary, confirm the change 
	if(this->valueChanged){
		QMessageBox msgBox;
		msgBox.setWindowTitle(_("changes on current preset not saved"));
		msgBox.setText(
			QString(_("The current preset %1 contains changes, but is not saved yet. Do you really want to change the preset and lose your changes?"))
			.arg(comboBoxPreset->itemData(lastComboboxIndex).toString()));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::Cancel) {
			//be careful to not cause a recursion
			comboBoxPreset->setCurrentIndex(lastComboboxIndex);
			return;
		}
	}
	this->valueChanged=false;
	
	removeChangeIndicator();

	this->lastComboboxIndex = idx;
	defaultParameter();
	if(idx!=0){ //0 is "design default values"
		//apply the change
		const std::string v = comboBoxPreset->itemData(idx).toString().toUtf8().constData();
		applyParameterSet(v);
	}
	emit previewRequested(false);
}

//if the set name is changed to "" asks if the user want to delete the current preset
//if the set name is changed and  no  values are changed, rename the current preset
//if the set name is changed and some values are changed, create a new set
void ParameterWidget::onSetNameChanged(){
	this->comboBoxPreset->lineEdit()->blockSignals(true); //prevent double firing

	int idx =  comboBoxPreset->currentIndex();

	QString newName = this->comboBoxPreset->currentText();
	QString oldName = comboBoxPreset->itemData(idx).toString().toUtf8().constData();
	if(oldName == newName){
		//nothing to do
	}else if(oldName ==""){
		//ignore
	}else if(newName =="" && idx!=0){
		QMessageBox msgBox;
		msgBox.setWindowTitle(_("Do you want to delete the current preset?"));
		msgBox.setText(
			QString(_("Do you want to delete the current preset '%1'?"))
			.arg(oldName));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);

		if (msgBox.exec() == QMessageBox::Cancel) {
			comboBoxPreset->lineEdit()->setText(oldName);
		}else{
			onSetDelete();
		}
	}else{
		if(!this->valueChanged){
			boost::optional<pt::ptree &> sets = this->setMgr->parameterSets();
			if (sets.is_initialized()) {
				sets.get().erase(pt::ptree::key_type(oldName.toStdString()));
			}
		}

		if (this->setMgr->isEmpty()) {
			pt::ptree setRoot;
			this->setMgr->addChild(ParameterSet::parameterSetsKey, setRoot);
		}

		updateParameterSet(newName.toStdString(),true);

		this->comboBoxPreset->clear();
		setComboBoxPresetForSet();
		this->comboBoxPreset->setCurrentIndex(this->comboBoxPreset->findData(newName));
	}
	this->comboBoxPreset->lineEdit()->blockSignals(false);
}

void ParameterWidget::onDescriptionLoDChanged()
{
	descriptionLoD =static_cast<DescLoD>(comboBoxDetails->currentIndex());
	emit previewRequested();
}

void ParameterWidget::onValueChanged()
{
	if(!this->valueChanged){
		this->labelChangeIndicator->setText("*");
	}
	this->valueChanged=true;

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
			for(unsigned int i=0; i < gr.size(); ++i) {
				ParameterVirtualWidget * entry = CreateParameterWidget(gr[i]);
				if(entry){
					anyLayout->addWidget(entry);
				}
			}

			GroupWidget *groupWidget = new GroupWidget(groupMap[groupName].show, QString::fromStdString(groupName));
			groupWidget->setContentLayout(*anyLayout);
			this->scrollAreaWidgetContents->layout()->addWidget(groupWidget);
		}
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
	for (unsigned int it=0; it<ParameterPos.size(); ++it) {
		std::string groupName=entries[ParameterPos[it]]->groupName;
		if (groupMap.find(groupName) == groupMap.end()) {
			groupPos.push_back(groupName);
			groupInst enter;
			enter.parameterVector.push_back(ParameterPos[it]);
			enter.show = false;
			enter.inList=true;
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
			entry = new ParameterComboBox(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::SLIDER:{
			entry = new ParameterSlider(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::CHECKBOX:{
			entry = new ParameterCheckBox(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::TEXT:{
			entry = new ParameterText(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::NUMBER:{
			entry = new ParameterSpinBox(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::VECTOR:{
			entry = new ParameterVector(this, entries[parameterName], this->descriptionLoD);
			break;
		}
		case ParameterObject::UNDEFINED:{
			break;
		}
    }
    if (entry) {
		connect(entry, SIGNAL(changed()), this, SLOT(onValueChanged()));
	}
	return entry;
}

//reset all parameters to the default value of the design file
void ParameterWidget::defaultParameter(){
	for (const auto &entry : entries) {
		entry.second->value=entry.second->defaultValue.clone();
	}
}

void ParameterWidget::applyParameterSet(std::string setName)
{
	boost::optional<pt::ptree &> set = this->setMgr->getParameterSet(setName);
	if (!set.is_initialized()) {
		return;
	}

	for (pt::ptree::value_type &v : set.get()) {
		entry_map_t::iterator entry = entries.find(v.first);
		if (entry != entries.end()) {
			if (entry->second->dvt == Value::Type::STRING) {
				entry->second->value = Value(v.second.data());
			} else if (entry->second->dvt == Value::Type::BOOL) {
				entry->second->value = Value(v.second.get_value<bool>());
			} else {
				shared_ptr<Expression> params = CommentParser::parser(v.second.data().c_str());
				if (!params) continue;
				
				ContextHandle<Context> ctx{Context::create<Context>()};
				Value newValue = params->evaluate(ctx.ctx);
				if (entry->second->dvt == newValue.type()) {
					entry->second->value = std::move(newValue);
				}
			}
		}
	}
}

void ParameterWidget::updateParameterSet(std::string setName, bool newSet)
{
	if (this->setMgr->isEmpty()) {
		pt::ptree setRoot;
		this->setMgr->addChild(ParameterSet::parameterSetsKey, setRoot);
	}

	if (newSet && setName == "") {
		QInputDialog *setDialog = new QInputDialog();

		bool ok = true;
		const QString result = setDialog->getText(this,
			_("Create new set of parameter"), _("Enter name of the parameter set"),
			QLineEdit::Normal, "", &ok);

		if (ok) {
			setName = result.trimmed().toStdString();
		}
	}

	//check for duplicates
	if(newSet && this->setMgr->setNameExists(setName)){
		QMessageBox msgBox;
		msgBox.setWindowTitle(QString(_("Set Name %1 already exists")).arg(QString::fromStdString(setName)));
		msgBox.setText(QString(_("The set name  %1 already exists. Do you want overwrite it?")).arg(QString::fromStdString(setName)));
		msgBox.setStandardButtons(QMessageBox::Yes);
		msgBox.addButton(QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);

		if (msgBox.exec() == QMessageBox::Yes) {
			//delete the preexisting preset to avoid side efects
			boost::optional<pt::ptree &> sets = this->setMgr->parameterSets();
			if (sets.is_initialized()) {
				sets.get().erase(pt::ptree::key_type(setName));
			}
			
			this->comboBoxPreset->removeItem(this->comboBoxPreset->findData(QString::fromStdString(setName)));
		}else{
			setName = "";
		}
	}

	if (!setName.empty()) {
		this->valueChanged=false;

		pt::ptree iroot;
		for (const auto &entry : entries) {
			const auto &VariableName = entry.first;
			const auto &VariableValue = entry.second->value.toString();
			iroot.put(VariableName, VariableValue);
		}
		this->setMgr->addParameterSet(setName, iroot);
		const QString s(QString::fromStdString(setName));
		const int idx = this->comboBoxPreset->findData(s);
		if (idx == -1) {
			this->comboBoxPreset->addItem(s, QVariant(s));
			this->comboBoxPreset->setCurrentIndex(this->comboBoxPreset->findData(s));
		}else{
			removeChangeIndicator();
		}
		writeParameterSets();
	}
}

void ParameterWidget::writeParameterSets()
{
	if(this->unreadableFileExists){
		QMessageBox msgBox;
		msgBox.setWindowTitle(_("Saving presets"));
		msgBox.setText(QString(_("%1 was found, but was unreadable. Do you want to overwrite %1?")).arg(QString::fromStdString(this->jsonFile)));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);

		if (msgBox.exec() == QMessageBox::Cancel) {
			return;
		}
	}
	this->setMgr->writeParameterSet(this->jsonFile);
	this->valueChanged=false;
}

void ParameterWidget::removeChangeIndicator()
{
	this->labelChangeIndicator->setText("");
}

bool ParameterWidget::childHasFocus(){
	if(this->hasFocus()) return true;
	auto children = this->findChildren<QWidget *>();
	for (auto child : children) {
		if(child->hasFocus()) return true;
	}
	return false;
}
