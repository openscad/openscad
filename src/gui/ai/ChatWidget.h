#pragma once

#include <QWidget>
#include <memory>
#include <optional>
#include <vector>
#include "core/AIService.h"
#include "gui/qtgettext.h"  // IWYU pragma: keep
#include "ui_ChatWidget.h"

class QLabel;
class QMenu;
class QTimer;

class MessageBubble : public QWidget
{
  Q_OBJECT
public:
  MessageBubble(const QString& text, bool isUser, const std::vector<ImageAttachment>& images = {},
                QWidget *parent = nullptr);
  void updateText(const QString& text);

private:
  bool isDarkTheme() const;
  QLabel *label;
  QTimer *thinkingTimer = nullptr;
  int thinkingStep = 0;
};

class CollapsibleBubble;

class ChatWidget : public QWidget, public Ui::ChatWidget
{
  Q_OBJECT

public:
  ChatWidget(QWidget *parent = nullptr);
  virtual ~ChatWidget();

  void proposeCodeChange(const std::string& code);
  bool hasPendingCodeChanges() const;
  void logToolExecution(const std::string& name, const std::string& result);
  void startNewResponseTurn();

private slots:
  void onSendPressed();
  void onClearPressed();
  void exportChat();
  void importChat();
  void copyAsMarkdown();
  void onAnalyzeScreenPressed();
  void onAttachImagePressed();
  void removeAttachment(size_t index);

private:
  MessageBubble *addMessage(const QString& text, bool isUser,
                            const std::vector<ImageAttachment>& images = {});
  void rebuildChatUI();
  void updateAttachmentPreviewBar();
  bool isDarkTheme() const;
  void enableInput(bool enabled);
  std::string executeTool(const std::string& name, const std::string& arguments_json);

  std::shared_ptr<AIService> aiService;
  std::vector<ChatMessage> history;
  std::shared_ptr<bool> aliveState;

  MessageBubble *activeAIBubble = nullptr;
  std::shared_ptr<std::string> activeResponseText;
  bool isRequestRunning = false;

  std::string proposedCode;
  std::string originalCode;
  QWidget *diffBannerWidget = nullptr;
  CollapsibleBubble *activeToolBubble = nullptr;
  std::vector<ImageAttachment> pendingAttachments;
  bool agenticMode = true;          // true = agentic (auto-loop), false = interactive review
  bool pendingAutoPreview = false;  // fire preview automatically after user accepts diff
  std::optional<ChatMessage> pendingViewportSnapshot;  // viewport image to inject before next agentic turn
};
