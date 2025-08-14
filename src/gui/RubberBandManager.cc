#include "RubberBandManager.h"

#include "MainWindow.h"
#include "Dock.h"

RubberBandManager::RubberBandManager(MainWindow *w) : rubberBand(QRubberBand::Rectangle)
{
  setParent(w);
  emphasizedDock = nullptr;
}

void RubberBandManager::hide()
{
  rubberBand.hide();
  emphasizedDock = nullptr;
}

bool RubberBandManager::isEmphasized(Dock *dock)
{
  return rubberBand.isVisible() && emphasizedDock == dock;
}

bool RubberBandManager::isVisible() { return rubberBand.isVisible(); }

void RubberBandManager::emphasize(Dock *dock)
{
  rubberBand.setParent(dock);
  rubberBand.setGeometry(dock->widget()->geometry());
  rubberBand.show();
  emphasizedDock = dock;
}
