/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
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
#include <QListWidgetItem>

#include <boost/foreach.hpp>

#include "qtgettext.h"
#include "LibraryWidget.h"
#include "ParameterEntryWidget.h"

#include "module.h"
#include "modcontext.h"
#include "expression.h"
#include "ModuleCache.h"

LibraryWidget::LibraryWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);
}

LibraryWidget::~LibraryWidget()
{
}

void LibraryWidget::onValueChanged()
{
	QList<QListWidgetItem *> items = listWidgetParts->selectedItems();
	if (items.isEmpty()) {
		return;
	}

	std::stringstream stream;

	LibraryEntry *entry = static_cast<LibraryEntry *>(items.first()->data(Qt::UserRole).value<void *>());
	stream << entry->moduleName << "(";
	bool first = true;
	for (entry_map_t::iterator it = widgets.begin();it != widgets.end();it++) {
		if ((*it).second->isDefaultValue()) {
			continue;
		}
		if (!first) {
			stream << ", ";
		}
		first = false;
		stream << (*it).first << " = " << *((*it).second->getValue());
	}
	stream << ");";

	listWidgetParts->setDragText(QString::fromStdString(stream.str()));
}

void LibraryWidget::on_listWidgetParts_itemSelectionChanged()
{
	clearParams();

	QList<QListWidgetItem *> items = listWidgetParts->selectedItems();
	if (items.size() == 0) {
		listWidgetParts->setDragText("");
		return;
	}

	LibraryEntry *entry = static_cast<LibraryEntry *>(items.first()->data(Qt::UserRole).value<void *>());

	ModuleContext ctx;
	
	foreach(Assignment assignment, entry->assignments) {
		ParameterEntryWidget *widget = new ParameterEntryWidget();
		connect(widget, SIGNAL(changed()), this, SLOT(onValueChanged()));
		const ValuePtr defaultValue = assignment.second.get()->evaluate(&ctx);
		widget->setAssignment(&ctx, &assignment, defaultValue);
		addEntry(widget);
		widgets[assignment.first] = widget;
	}

	end();

	onValueChanged();
}

void LibraryWidget::clearParams()
{
	QLayoutItem *child;
	while ((child = this->scrollAreaWidgetContents->layout()->takeAt(0)) != 0) {
		QWidget *w = child->widget();
		this->scrollAreaWidgetContents->layout()->removeWidget(w);
		delete w;
	}

	widgets.clear();
}

void LibraryWidget::addEntry(ParameterEntryWidget *entry)
{
	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	policy.setVerticalPolicy(QSizePolicy::Preferred);
	policy.setHorizontalStretch(0);
	policy.setVerticalStretch(0);
	entry->setSizePolicy(policy);
	this->scrollAreaWidgetContents->layout()->addWidget(entry);
}

void LibraryWidget::end()
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

void LibraryWidget::applyModuleParameters(const std::string& name, Module *module)
{
	if (module == NULL) {
		return;
	}

	ModuleContext ctx;
	const Annotation *part = module->annotation("LibraryPart");
	if (part == NULL) {
		return;
	}

	const ValuePtr partName = part->evaluate(&ctx, "name");

	QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(partName->toString()), listWidgetParts);

	LibraryEntry *entry = new LibraryEntry();
	entries.push_back(LibraryEntry::ptr(entry));
	entry->moduleName = name;
	item->setData(Qt::UserRole, qVariantFromValue((void *)entry));
	listWidgetParts->addItem(item);

	foreach(Assignment assignment, module->definition_arguments)
	{
		const Annotation *param = assignment.annotation("Parameter");
		if ((param == NULL) || (assignment.second == NULL)) {
			continue;
		}

		const ValuePtr defaultValue = assignment.second.get()->evaluate(&ctx);
		if (defaultValue->type() == Value::UNDEFINED) {
			continue;
		}
		
		entry->assignments.push_back(assignment);
	}
}

void LibraryWidget::setParameters(Module *module)
{
	listWidgetParts->clear();
	clearParams();
	applyParameters(module);
}

void LibraryWidget::applyParameters(Module *module)
{
	if (module == NULL) {
		return;
	}

	for (LocalScope::AbstractModuleContainer::iterator it = module->scope.modules.begin();it != module->scope.modules.end();it++) {
		applyModuleParameters((*it).first, dynamic_cast<Module *>((*it).second));
	}

	FileModule *fileModule = dynamic_cast<FileModule *>(module);
	if (fileModule) {
		BOOST_FOREACH(const FileModule::ModuleContainer::value_type &m, fileModule->usedlibs) {
			FileModule *usedmod = ModuleCache::instance()->lookup(m);
			if (usedmod)
				applyParameters(usedmod);
		}
	}
}
