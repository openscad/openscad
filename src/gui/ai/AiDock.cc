#include "gui/ai/AIDock.h"
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWidget>

AIDock::AIDock(QWidget *parent) : Dock(parent)
{
  this->centralWidget = new QWidget(this);
  this->layout = new QVBoxLayout(this->centralWidget);

  setWidget(this->centralWidget);
}

AIDock::~AIDock()
{
}

void AIDock::showEvent(QShowEvent *event)
{
  Dock::showEvent(event);
}
