#include "WheelIgnorer.h"

WheelIgnorer::WheelIgnorer(QWidget *parent) : QObject(parent)
{
}

bool WheelIgnorer::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Wheel){
                return true;
    }
    return QObject::eventFilter(obj, event);
}
