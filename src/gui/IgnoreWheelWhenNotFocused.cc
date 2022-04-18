#include "IgnoreWheelWhenNotFocused.h"
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

void installIgnoreWheelWhenNotFocused(QWidget *parent){
  auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(parent);

  auto comboBoxes = parent->findChildren<QComboBox *>();
  for (auto comboBox : comboBoxes) {
    comboBox->installEventFilter(ignoreWheelWhenNotFocused);
    comboBox->setFocusPolicy(Qt::StrongFocus);
  }

  auto spinBoxes = parent->findChildren<QSpinBox *>();
  for (const auto& spinBox : spinBoxes){
      spinBox->installEventFilter(ignoreWheelWhenNotFocused);
      spinBox->setFocusPolicy(Qt::StrongFocus);
  }

  auto spinDoubleBoxes = parent->findChildren<QDoubleSpinBox *>();
  for (auto spinDoubleBox : spinDoubleBoxes) {
    spinDoubleBox->installEventFilter(ignoreWheelWhenNotFocused);
    spinDoubleBox->setFocusPolicy(Qt::StrongFocus);
  }

  // clang generates a bogus warning that wheelIgnorer may be leaked
}

IgnoreWheelWhenNotFocused::IgnoreWheelWhenNotFocused(QWidget *parent) : QObject(parent)
{
}

//https://stackoverflow.com/questions/5821802/qspinbox-inside-a-qscrollarea-how-to-prevent-spin-box-from-stealing-focus-when
bool IgnoreWheelWhenNotFocused::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Wheel) {
    if (!((QWidget *)obj)->hasFocus()) {
      return true;
    } else {
      return QObject::eventFilter(obj, event);
    }
  } else {
    return QObject::eventFilter(obj, event);
  }
}

