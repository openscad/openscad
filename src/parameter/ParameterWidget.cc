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

#include "GroupWidget.h"
#include "ParameterSpinBox.h"
#include "ParameterComboBox.h"
#include "ParameterSlider.h"
#include "ParameterCheckBox.h"
#include "ParameterText.h"
#include "ParameterVector.h"

#include <boost/filesystem.hpp>

#include <QInputDialog>
#include <QMessageBox>

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
	scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop);

	autoPreviewTimer.setInterval(1000);
	autoPreviewTimer.setSingleShot(true);

	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(emitParametersChanged()));
	connect(checkBoxAutoPreview, &QCheckBox::toggled, [this]() { this->autoPreview(true); } );
	connect(comboBoxDetails, SIGNAL(currentIndexChanged(int)), this, SLOT(rebuildWidgets()));
	connect(comboBoxPreset, SIGNAL(activated(int)), this, SLOT(onSetChanged(int)));
	//connect(comboBoxPreset, SIGNAL(editTextChanged(const QString&)), this, SLOT(onSetNameChanged()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(onSetAdd()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(onSetDelete()));
}

// Can only be called before the initial setParameters().
void ParameterWidget::readFile(QString scadFile)
{
	assert(sets.empty());
	assert(parameters.empty());
	assert(widgets.empty());

	QString jsonFile = getJsonFile(scadFile);
	if (!boost::filesystem::exists(jsonFile.toStdString())) {
		this->invalidJsonFile = QString();
	} else if (this->sets.readFile(jsonFile.toStdString())) {
		this->invalidJsonFile = QString();
	} else {
		this->invalidJsonFile = jsonFile;
	}

	for (const auto& set : this->sets) {
		comboBoxPreset->addItem(QString::fromStdString(set.name()));
	}
}

// Write the json file if the parameter sets are not empty.
// This prevents creating unnecessary json files.
void ParameterWidget::saveFile(QString scadFile)
{
	if (sets.empty()) {
		return;
	}

	QString jsonFile = getJsonFile(scadFile);
	if (jsonFile == this->invalidJsonFile) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(_("Saving presets"));
		msgBox.setText(QString(_("%1 was found, but was unreadable. Do you want to overwrite %1?")).arg(this->invalidJsonFile));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::Cancel) {
			return;
		}
	}

	cleanSets();
	sets.writeFile(jsonFile.toStdString());
}

void ParameterWidget::saveBackupFile(QString scadFile)
{
	if (sets.empty()) {
		return;
	}

	sets.writeFile(getJsonFile(scadFile).toStdString());
}

void ParameterWidget::setParameters(const SourceFile* sourceFile, const std::string& source)
{
	if (this->source == source) {
		return;
	}
	this->source = source;

	this->parameters = ParameterObjects::fromSourceFile(sourceFile);
	rebuildWidgets();
	loadSet(comboBoxPreset->currentIndex());
}

void ParameterWidget::applyParameters(SourceFile *sourceFile)
{
	this->parameters.apply(sourceFile);
}

bool ParameterWidget::childHasFocus()
{
	if(this->hasFocus()) {
		return true;
	}
	auto children = this->findChildren<QWidget *>();
	for (auto child : children) {
		if(child->hasFocus()) {
			return true;
		}
	}
	return false;
}

void ParameterWidget::setModified(bool modified)
{
	if (this->modified != modified) {
		this->modified = modified;
		emit modificationChanged();
	}
}

void ParameterWidget::emitParametersChanged() {
	for (const auto &kvp : widgets) {
		for (ParameterVirtualWidget* widget : kvp.second) {
			widget->valueApplied();
		}
	}
	emit parametersChanged();
}

void ParameterWidget::autoPreview(bool immediate)
{
	autoPreviewTimer.stop();
	if (checkBoxAutoPreview->isChecked()) {
		if (immediate) {
			emitParametersChanged();
		} else {
			autoPreviewTimer.start();
		}
	}
}

void ParameterWidget::onSetChanged(int index)
{
	loadSet(index);
	autoPreview(true);
}

void ParameterWidget::onSetNameChanged()
{
	assert(comboBoxPreset->count() == sets.size() + 1);
	comboBoxPreset->setItemText(comboBoxPreset->currentIndex(), comboBoxPreset->lineEdit()->text());
	sets[comboBoxPreset->currentIndex() - 1].setName(comboBoxPreset->currentText().toStdString());
	setModified();
}

void ParameterWidget::onSetAdd()
{
	bool ok = true;
	QString result = QInputDialog::getText(
		this,
		_("Create new set of parameter"),
		_("Enter name of the parameter set"),
		QLineEdit::Normal,
		"",
		&ok
	);

	if (ok) {
		createSet(result.trimmed());
	}
	setModified();
}

void ParameterWidget::onSetDelete()
{
	int index = comboBoxPreset->currentIndex();
	assert(index > 0);
	int newIndex;
	if (index + 1 == comboBoxPreset->count()) {
		newIndex = index - 1;
	} else {
		newIndex = index + 1;
	}
	comboBoxPreset->setCurrentIndex(newIndex);
	loadSet(newIndex);

	comboBoxPreset->removeItem(index);
	sets.erase(sets.begin() + (index - 1));
	setModified();
	autoPreview(true);
}

void ParameterWidget::parameterModified(bool immediate)
{
	ParameterVirtualWidget* widget = (ParameterVirtualWidget*)sender();
	ParameterObject* parameter = widget->getParameter();

	// When attempting to modify the design default, create a new set to edit.
	if (comboBoxPreset->currentIndex() == 0) {
		std::set<std::string> setNames;
		for (const auto& set : this->sets) {
			setNames.insert(set.name());
		}

		QString name;
		for (int i = 1; ; i++) {
			name = _("New set ") + QString::number(i);
			if (setNames.count(name.toStdString()) == 0) {
				break;
			}
		}
		createSet(name);
	}

	size_t setIndex = comboBoxPreset->currentIndex() - 1;
	assert(setIndex < sets.size());
	sets[setIndex][parameter->name()] = parameter->exportValue();

	assert(widgets.count(parameter) == 1);
	for (ParameterVirtualWidget* otherWidget : widgets[parameter]) {
		if (otherWidget != widget) {
			otherWidget->setValue();
		}
	}

	setModified();
	autoPreview(immediate);
}

void ParameterWidget::loadSet(int index)
{
	assert(index <= sets.size());
	if (index == 0) {
		parameters.reset();
	} else {
		parameters.importValues(sets[index - 1]);
	}

	updateSetEditability();

	for (const auto& pair : widgets) {
		for (ParameterVirtualWidget* widget : pair.second) {
			widget->setValue();
		}
	}
}

void ParameterWidget::createSet(QString name)
{
	sets.push_back(parameters.exportValues(name.toStdString()));
	comboBoxPreset->addItem(name);
	comboBoxPreset->setCurrentIndex(comboBoxPreset->count() - 1);
	updateSetEditability();
}

void ParameterWidget::updateSetEditability()
{
	if (comboBoxPreset->currentIndex() == 0) {
		comboBoxPreset->setEditable(false);
		deleteButton->setEnabled(false);
	} else {
		if (!comboBoxPreset->isEditable()) {
			comboBoxPreset->setEditable(true);
			connect(comboBoxPreset->lineEdit(), SIGNAL(textEdited(const QString&)), this, SLOT(onSetNameChanged()));
		}
		deleteButton->setEnabled(true);
	}
}

void ParameterWidget::rebuildWidgets()
{
	std::map<QString, bool> expandedGroups;
	for (GroupWidget* groupWidget : this->findChildren<GroupWidget*>()) {
		expandedGroups.emplace(groupWidget->title(), groupWidget->isExpanded());
	}

	widgets.clear();
	QLayout* layout = this->scrollAreaWidgetContents->layout();
	while (layout->count() > 0) {
		QLayoutItem* child = layout->takeAt(0);
		delete child->widget();
		delete child;
	}

	DescriptionStyle descriptionStyle = static_cast<DescriptionStyle>(comboBoxDetails->currentIndex());
	std::vector<ParameterGroup> parameterGroups = getParameterGroups();
	for (const auto& group : parameterGroups) {
		GroupWidget* groupWidget = new GroupWidget(group.name);
		for (ParameterObject* parameter : group.parameters) {
			ParameterVirtualWidget* parameterWidget = createParameterWidget(parameter, descriptionStyle);
			connect(parameterWidget, SIGNAL(changed(bool)), this, SLOT(parameterModified(bool)));
			if (!widgets.count(parameter)) {
				widgets[parameter] = {};
			}
			widgets[parameter].push_back(parameterWidget);
			groupWidget->addWidget(parameterWidget);
		}
		auto it = expandedGroups.find(group.name);
		groupWidget->setExpanded(it == expandedGroups.end() || it->second);
		layout->addWidget(groupWidget);
	}
}

std::vector<ParameterWidget::ParameterGroup> ParameterWidget::getParameterGroups()
{
	std::vector<ParameterWidget::ParameterGroup> output;
	std::map<std::string, size_t> groupIndices;
	std::vector<ParameterObject*> globalParameters;

	for (const std::unique_ptr<ParameterObject>& parameter : parameters) {
		std::string group = parameter->group();
		if (group == "Global") {
			globalParameters.push_back(parameter.get());
		} else if (group == "Hidden") {
			continue;
		} else {
			if (!groupIndices.count(group)) {
				groupIndices[group] = output.size();
				output.push_back({QString::fromStdString(group)});
			}
			output[groupIndices[group]].parameters.push_back(parameter.get());
		}
	}

	if (output.size() == 0 && globalParameters.size() > 0) {
		ParameterGroup global;
		global.name = "Global";
		global.parameters = std::move(globalParameters);
		output.push_back(std::move(global));
	} else {
		for (auto& group : output) {
			group.parameters.insert(group.parameters.end(), globalParameters.begin(), globalParameters.end());
		}
	}

	return output;
}

ParameterVirtualWidget* ParameterWidget::createParameterWidget(ParameterObject* parameter, DescriptionStyle descriptionStyle)
{
	if (parameter->type() == ParameterObject::ParameterType::Bool) {
		return new ParameterCheckBox(this, static_cast<BoolParameter*>(parameter), descriptionStyle);
	} else if (parameter->type() == ParameterObject::ParameterType::String) {
		return new ParameterText(this, static_cast<StringParameter*>(parameter), descriptionStyle);
	} else if (parameter->type() == ParameterObject::ParameterType::Number) {
		NumberParameter* numberParameter = static_cast<NumberParameter*>(parameter);
		if (numberParameter->minimum && numberParameter->maximum) {
			return new ParameterSlider(this, numberParameter, descriptionStyle);
		} else {
			return new ParameterSpinBox(this, numberParameter, descriptionStyle);
		}
	} else if (parameter->type() == ParameterObject::ParameterType::Vector) {
		return new ParameterVector(this, static_cast<VectorParameter*>(parameter), descriptionStyle);
	} else if (parameter->type() == ParameterObject::ParameterType::Enum) {
		return new ParameterComboBox(this, static_cast<EnumParameter*>(parameter), descriptionStyle);
	} else {
		assert(false);
		throw std::runtime_error("Unsupported parameter widget type");
	}
}

QString ParameterWidget::getJsonFile(QString scadFile)
{
	boost::filesystem::path p = scadFile.toStdString();
	return QString::fromStdString(p.replace_extension(".json").string());
}

// Remove set values that do not correspond to a parameter,
// or that cannot be parsed as such.
void ParameterWidget::cleanSets()
{
	std::map<std::string, ParameterObject*> namedParameters;
	for (const auto& parameter : parameters) {
		namedParameters[parameter->name()] = parameter.get();
	}
	
	for (ParameterSet& set : sets) {
		for (auto it = set.begin(); it != set.end(); ) {
			if (!namedParameters.count(it->first)) {
				it = set.erase(it);
			} else {
				if (namedParameters[it->first]->importValue(it->second, false)) {
					++it;
				} else {
					it = set.erase(it);
				}
			}
		}
	}
}
