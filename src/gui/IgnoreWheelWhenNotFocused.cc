#include "gui/IgnoreWheelWhenNotFocused.h"
#include <QEvent>
#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

void installIgnoreWheelWhenNotFocused(QWidget *parent){
  auto comboBoxes = parent->findChildren<QComboBox *>();
  auto spinBoxes = parent->findChildren<QSpinBox *>();
  auto spinDoubleBoxes = parent->findChildren<QDoubleSpinBox *>();

  if(comboBoxes.size() == 0 && spinBoxes.size() == 0 && spinDoubleBoxes.size() == 0){
    return; //nothing do
  }

  auto *ignoreWheelWhenNotFocused = new IgnoreWheelWhenNotFocused(parent);

  for (auto comboBox : comboBoxes) {
    comboBox->installEventFilter(ignoreWheelWhenNotFocused);
    comboBox->setFocusPolicy(Qt::StrongFocus);
  }

  for (const auto& spinBox : spinBoxes){
      spinBox->installEventFilter(ignoreWheelWhenNotFocused);
      spinBox->setFocusPolicy(Qt::StrongFocus);
  }

  for (auto spinDoubleBox : spinDoubleBoxes) {
    spinDoubleBox->installEventFilter(ignoreWheelWhenNotFocused);
    spinDoubleBox->setFocusPolicy(Qt::StrongFocus);
  }

  // clang generates a bogus warning that ignoreWheelWhenNotFocused may be leaked
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

