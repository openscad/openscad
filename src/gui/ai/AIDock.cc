#include "gui/ai/AIDock.h"
#include "gui/ai/ChatWidget.h"
#include "Feature.h"
#include <QShowEvent>

AIDock::AIDock(QWidget *parent) : Dock(parent)
{
  this->setObjectName("aiDock");
}

AIDock::~AIDock()
{
}

void AIDock::showEvent(QShowEvent *event)
{
  Dock::showEvent(event);
  if (chatWidget == nullptr && Feature::ExperimentalAiFeatures.is_enabled()) {
    chatWidget = new ChatWidget(this);
    setWidget(chatWidget);
  }
}
