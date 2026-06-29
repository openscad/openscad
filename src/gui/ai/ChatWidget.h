#pragma once

#include <QWidget>
#include <memory>
#include <vector>
#include "core/AIService.h"
#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ChatWidget.h"

class QLabel;
class QTimer;

class MessageBubble : public QWidget
{
  Q_OBJECT
public:
  MessageBubble(const QString& text, bool isUser, QWidget *parent = nullptr);
  void updateText(const QString& text);

private:
  bool isDarkTheme() const;
  QLabel *label;
  QTimer *thinkingTimer = nullptr;
  int thinkingStep = 0;
};

class ChatWidget : public QWidget, public Ui::ChatWidget
{
  Q_OBJECT

public:
  ChatWidget(QWidget *parent = nullptr);
  virtual ~ChatWidget();

  void proposeCodeChange(const std::string& code);
  bool hasPendingCodeChanges() const;

private slots:
  void onSendPressed();
  void onClearPressed();

private:
  MessageBubble *addMessage(const QString& text, bool isUser);
  bool isDarkTheme() const;
  void enableInput(bool enabled);

  std::shared_ptr<AIService> aiService;
  std::vector<ChatMessage> history;
  std::shared_ptr<bool> aliveState;

  MessageBubble *activeAIBubble = nullptr;
  std::shared_ptr<std::string> activeResponseText;
  bool isRequestRunning = false;

  std::string proposedCode;
  std::string originalCode;
  QWidget *diffBannerWidget = nullptr;
};
