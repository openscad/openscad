//This event filter ignores Mouse Wheel events when the object is not focused.
//A lot of elements in OpenSCAD are in Scroll Areas.
//This causes a conflict, as some elements within the Scroll Areas are
//also reacting to the mousewheel.
//Especially in the settings, where the user might spend a considerable
//amount of time to get it just right, it is annoying when simply
//scrowling down unintentionally changes various settings.

//for reference:
//https://stackoverflow.com/questions/5821802/qspinbox-inside-a-qscrollarea-how-to-prevent-spin-box-from-stealing-focus-when
//http://doc.qt.io/archives/qt-4.8/qobject.html#installEventFilter

#include <QWidget>
#include <QMouseEvent>

class IgnoreWheelWhenNotFocused : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

