#pragma once

#include <QPlainTextEdit>

class QKeyEvent;

class ChatInputEdit : public QPlainTextEdit
{
  Q_OBJECT

public:
  ChatInputEdit(QWidget *parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent *event) override;

signals:
  void sendPressed();
};
