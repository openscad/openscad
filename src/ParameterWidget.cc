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
#include "ParameterEntryWidget.h"

#include "module.h"
#include "modcontext.h"
#include "expression.h"

ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
        connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
}

ParameterWidget::~ParameterWidget()
{
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

	entries.clear();
}

void ParameterWidget::addEntry(ParameterEntryWidget *entry)
{
	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Expanding);
	policy.setVerticalPolicy(QSizePolicy::Preferred);
	policy.setHorizontalStretch(0);
	policy.setVerticalStretch(0);
	entry->setSizePolicy(policy);
	this->scrollAreaWidgetContents->layout()->addWidget(entry);
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

void ParameterWidget::applyParameters(FileModule *fileModule)
{
	if (fileModule == NULL) {
		return;
	}

	for (AssignmentList::iterator it = fileModule->scope.assignments.begin();it != fileModule->scope.assignments.end();it++) {
		entry_map_t::iterator entry = entries.find((*it).first);
		if (entry == entries.end()) {
			continue;
		}

		(*entry).second->applyParameter(&(*it));
	}
}

void ParameterWidget::setParameters(const Module *module)
{
	if (module == NULL) {
		return;
	}

	ModuleContext ctx;

	begin();
	foreach(Assignment assignment, module->scope.assignments)
	{
		const Annotation *param = assignment.annotation("Parameter");
		if (!param) {
			continue;
		}

		const ValuePtr defaultValue = assignment.second.get()->evaluate(&ctx);
		if (defaultValue->type() == Value::UNDEFINED) {
			continue;
		}

		ParameterEntryWidget *entry = new ParameterEntryWidget();
		entry->setAssignment(&ctx, &assignment, defaultValue);
		connect(entry, SIGNAL(changed()), this, SLOT(onValueChanged()));
		addEntry(entry);
		entries[assignment.first] = entry;

	}
	end();
}
