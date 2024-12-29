#include "gui/parameter/GroupWidget.h"

#include <QObject>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QLineEdit>

GroupWidget::GroupWidget(const QString& title, QWidget *parent) : QWidget(parent)
{
  this->toggleButton.setText(title);
  this->toggleButton.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  this->toggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  this->toggleButton.setCheckable(true);
  setExpanded(false);

  // don't waste space
  this->mainLayout.setVerticalSpacing(0);
  this->mainLayout.setContentsMargins(0, 0, 0, 0);
  this->contentArea.setContentsMargins(0, 0, 0, 0);
  this->contentLayout.setSpacing(0);
  this->contentLayout.setContentsMargins(0, 0, 0, 0);

  this->contentArea.setLayout(&contentLayout);
  this->contentArea.hide();
  this->mainLayout.addWidget(&toggleButton, 0, 0);
  this->mainLayout.addWidget(&contentArea, 1, 0);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  setLayout(&mainLayout);

  QObject::connect(&toggleButton, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
}

void GroupWidget::addWidget(QWidget *widget)
{
  contentLayout.addWidget(widget);
}

void GroupWidget::setExpanded(bool expanded)
{
  toggleButton.setChecked(expanded);
  toggleButton.setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
  if (expanded) {
    contentArea.show();
  } else {
    contentArea.hide();
  }
}
