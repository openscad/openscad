//https://stackoverflow.com/questions/3241830/qt-how-to-disable-mouse-scrolling-of-qcombobox
//https://stackoverflow.com/questions/5821802/qspinbox-inside-a-qscrollarea-how-to-prevent-spin-box-from-stealing-focus-when
//http://doc.qt.io/archives/qt-4.8/qobject.html#installEventFilter
#include <QWidget>
#include <QMouseEvent>

class WheelIgnorer : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};
