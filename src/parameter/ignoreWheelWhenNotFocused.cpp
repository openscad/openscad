#include "ignoreWheelWhenNotFocused.h"

//https://stackoverflow.com/questions/5821802/qspinbox-inside-a-qscrollarea-how-to-prevent-spin-box-from-stealing-focus-when
bool IgnoreWheelWhenNotFocused::eventFilter(QObject *obj, QEvent *event)
{
	if(event->type() == QEvent::Wheel){
		if(!((QWidget*)obj)->hasFocus()){
			return true;
		}else{
			return QObject::eventFilter(obj, event);
		}
	}else{
		return QObject::eventFilter(obj, event);
	}
}

