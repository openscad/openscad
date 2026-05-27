#pragma once

#include <QWidget>
#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ChatWidget.h"

class MessageBubble : public QWidget
{
  Q_OBJECT
public:
  MessageBubble(const QString& text, bool isUser, QWidget *parent = nullptr);

private:
  bool isDarkTheme() const;
};

class ChatWidget : public QWidget, public Ui::ChatWidget
{
  Q_OBJECT

public:
  ChatWidget(QWidget *parent = nullptr);
  virtual ~ChatWidget();

private slots:
  void onSendPressed();
  void onClearPressed();
  void simulateAIResponse(const QString& prompt, QWidget *thinkingBubble);

private:
  QWidget *addMessage(const QString& text, bool isUser);
  bool isDarkTheme() const;
};
