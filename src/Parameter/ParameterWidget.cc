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


ParameterWidget::ParameterWidget(QWidget *parent) : QWidget(parent)
{
	setupUi(this);

    descriptionShow=true;
	autoPreviewTimer.setInterval(500);
	autoPreviewTimer.setSingleShot(true);
	connect(&autoPreviewTimer, SIGNAL(timeout()), this, SLOT(onPreviewTimerElapsed()));
    connect(checkBoxAutoPreview, SIGNAL(toggled(bool)), this, SLOT(onValueChanged()));
    connect(checkBoxDetailedDescription,SIGNAL(toggled(bool)),this,SLOT(onDescriptionShow()));
}

ParameterWidget::~ParameterWidget()
{
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
    for(entry_map_t::iterator it = entries.begin(); it != entries.end(); it++) {
        if(!(*it).second->set){
            entries.erase((*it).first);
            continue;
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

    begin();
    for(group_map::iterator it = groupMap.begin(); it != groupMap.end(); it++) {
        vector<string> gr;
        gr=it->second.parameterVector;
        if(gr.empty()){
            groupMap.erase((*it).first);
            continue;
        }
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


