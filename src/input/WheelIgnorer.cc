#include "WheelIgnorer.h"

bool WheelIgnorer::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Wheel){
                return true;
    }
    return QObject::eventFilter(obj, event);
}
