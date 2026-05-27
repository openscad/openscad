#include "gui/ai/AIDock.h"
#include "gui/ai/ChatWidget.h"
#include <QShowEvent>

AIDock::AIDock(QWidget *parent) : Dock(parent)
{
  this->chatWidget = new ChatWidget(this);
  setWidget(this->chatWidget);
}

AIDock::~AIDock()
{
}

void AIDock::showEvent(QShowEvent *event)
{
  Dock::showEvent(event);
}
