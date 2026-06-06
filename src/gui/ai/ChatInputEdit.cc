#include "gui/ai/ChatInputEdit.h"
#include <QKeyEvent>

ChatInputEdit::ChatInputEdit(QWidget *parent) : QPlainTextEdit(parent)
{
  setPlaceholderText(tr("Ask AI..."));
  setFixedHeight(50);
}

void ChatInputEdit::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    if (event->modifiers() & Qt::ShiftModifier) {
      QPlainTextEdit::keyPressEvent(event);
    } else {
      emit sendPressed();
    }
  } else {
    QPlainTextEdit::keyPressEvent(event);
  }
}
