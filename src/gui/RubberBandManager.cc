#include "RubberBandManager.h"

#include "MainWindow.h"
#include "Dock.h"

RubberBandManager::RubberBandManager(MainWindow *w) :
  rubberBand(QRubberBand::Rectangle)
{
  setParent(w);
  w->installEventFilter(this);
}
bool RubberBandManager::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyRelease) {
    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Control && rubberBand.isVisible()) {
      hide();
    }
  }
  return false;
}

void RubberBandManager::hide(){
  rubberBand.hide();
}

void RubberBandManager::emphasize(Dock *dock){
  parent()->removeEventFilter(this);
  dock->installEventFilter(this);
  rubberBand.setParent(dock);
  rubberBand.setGeometry(dock->widget()->geometry());
  rubberBand.show();
}
