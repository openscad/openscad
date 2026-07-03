#include "gui/ai/ChatWidget.h"
#include "gui/qtgettext.h"

#include <QScrollBar>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QPalette>
#include "gui/MainWindow.h"
#include "gui/ai/DiffDialog.h"
#include "gui/OpenSCADApp.h"
#include "gui/ai/CollapsibleBubble.h"

// MessageBubble implementation
MessageBubble::MessageBubble(const QString& text, bool isUser, QWidget *parent) : QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 4, 0, 4);

  QFrame *bubbleFrame = new QFrame(this);
  bubbleFrame->setFrameShape(QFrame::StyledPanel);

  bool dark = isDarkTheme();
  QString frameStyle;
  QString labelStyle;

  if (isUser) {
    // User bubble: vibrant blue
    if (dark) {
      frameStyle = "QFrame { background-color: #2563eb; border: none; border-radius: 12px; }";
      labelStyle = "QLabel { color: #ffffff; font-size: 10pt; }";
    } else {
      frameStyle = "QFrame { background-color: #3b82f6; border: none; border-radius: 12px; }";
      labelStyle = "QLabel { color: #ffffff; font-size: 10pt; }";
    }
    layout->addStretch(1);
    layout->addWidget(bubbleFrame);
  } else {
    // AI bubble: slate/gray
    if (dark) {
      frameStyle = "QFrame { background-color: #374151; border: none; border-radius: 12px; }";
      labelStyle = "QLabel { color: #f3f4f6; font-size: 10pt; }";
    } else {
      frameStyle = "QFrame { background-color: #f3f4f6; border: none; border-radius: 12px; }";
      labelStyle = "QLabel { color: #1f2937; font-size: 10pt; }";
    }
    layout->addWidget(bubbleFrame);
    layout->addStretch(1);
  }

  bubbleFrame->setStyleSheet(frameStyle);

  QVBoxLayout *frameLayout = new QVBoxLayout(bubbleFrame);
  frameLayout->setContentsMargins(10, 8, 10, 8);

  this->label = new QLabel(text, bubbleFrame);
  this->label->setWordWrap(true);
  this->label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  this->label->setStyleSheet(labelStyle);

  frameLayout->addWidget(this->label);

  if (!isUser && text == _("Thinking...")) {
    thinkingStep = 0;
    thinkingTimer = new QTimer(this);
    connect(thinkingTimer, &QTimer::timeout, this, [this]() {
      thinkingStep = (thinkingStep + 1) % 4;
      QString dots;
      for (int i = 0; i < thinkingStep; ++i) {
        dots += ".";
      }
      this->label->setText(_("Thinking") + dots);
    });
    thinkingTimer->start(400);
  }
}

void MessageBubble::updateText(const QString& text)
{
  if (thinkingTimer) {
    thinkingTimer->stop();
    thinkingTimer->deleteLater();
    thinkingTimer = nullptr;
  }
  this->label->setText(text);
}

bool MessageBubble::isDarkTheme() const
{
  QPalette pal = QApplication::palette();
  return pal.color(QPalette::Window).lightness() < 128;
}

// ChatWidget implementation
ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent)
{
  setupUi(this);

  // Set titles/translations that might not be configured dynamically in UI files
  titleLabel->setText(_("AI Assistant"));
  clearButton->setText(_("Clear"));
  clearButton->setToolTip(_("Clear chat history"));
  sendButton->setText(_("Send"));

  // Connections
  connect(sendButton, &QPushButton::clicked, this, &ChatWidget::onSendPressed);
  connect(inputField, &ChatInputEdit::sendPressed, this, &ChatWidget::onSendPressed);
  connect(clearButton, &QPushButton::clicked, this, &ChatWidget::onClearPressed);

  // Initialize backend and state
  aiService = std::make_shared<AIService>();
  aliveState = std::make_shared<bool>(true);

  // Initial welcome greeting
  addMessage(_("Hello! I am your OpenSCAD AI assistant. Ask me to write some code, e.g. "
               "\"draw a sphere\" or \"create a box with a hole\"."),
             false);

  std::string defPrompt = aiService->getDefaultPrompt();
  if (!defPrompt.empty()) {
    inputField->setPlainText(QString::fromStdString(defPrompt));
  }

  // Initialize diff proposal banner
  diffBannerWidget = new QWidget(this);
  QHBoxLayout *bannerLayout = new QHBoxLayout(diffBannerWidget);
  bannerLayout->setContentsMargins(8, 4, 8, 4);
  bannerLayout->setSpacing(8);

  QLabel *bannerLabel = new QLabel(_("AI proposed code changes:"), diffBannerWidget);
  bannerLayout->addWidget(bannerLabel);

  QPushButton *reviewBtn = new QPushButton(_("Review & Apply"), diffBannerWidget);
  reviewBtn->setStyleSheet(
    "background-color: #2563eb; color: white; border-radius: 4px; padding: 4px 8px; font-weight: bold;");
  bannerLayout->addWidget(reviewBtn);

  QPushButton *discardBtn = new QPushButton(_("Discard"), diffBannerWidget);
  discardBtn->setStyleSheet(
    "background-color: #ef4444; color: white; border-radius: 4px; padding: 4px 8px; font-weight: bold;");
  bannerLayout->addWidget(discardBtn);

  bool dark = isDarkTheme();
  if (dark) {
    bannerLabel->setStyleSheet("font-weight: bold; color: #93c5fd;");
    diffBannerWidget->setStyleSheet(
      "background-color: #1e293b; border-top: 1px solid #334155; border-bottom: 1px solid #334155;");
  } else {
    bannerLabel->setStyleSheet("font-weight: bold; color: #1e3a8a;");
    diffBannerWidget->setStyleSheet(
      "background-color: #eff6ff; border-top: 1px solid #bfdbfe; border-bottom: 1px solid #bfdbfe;");
  }

  diffBannerWidget->hide();

  // Insert banner above the input field (index 2 in mainLayout)
  mainLayout->insertWidget(2, diffBannerWidget);

  // Connect banner actions
  connect(discardBtn, &QPushButton::clicked, this, [this]() {
    proposedCode = "";
    originalCode = "";
    diffBannerWidget->hide();
  });

  connect(reviewBtn, &QPushButton::clicked, this, [this]() {
    MainWindow *mw = nullptr;
    for (auto *win : scadApp->windowManager.getWindows()) {
      mw = win;
      break;
    }

    DiffDialog dlg(originalCode, proposedCode, isDarkTheme(), this);
    int result = dlg.exec();
    if (result == QDialog::Accepted) {
      if (mw && mw->activeEditor) {
        mw->activeEditor->setText(QString::fromStdString(proposedCode));
        if (dlg.shouldTriggerPreview()) {
          mw->actionRenderPreview();
        }
      }
      proposedCode = "";
      originalCode = "";
      diffBannerWidget->hide();
    } else if (result == 2) {  // Discarded Changes
      proposedCode = "";
      originalCode = "";
      diffBannerWidget->hide();
    }
  });
}

ChatWidget::~ChatWidget()
{
  *aliveState = false;
  aiService->cancelPendingRequests();
}

void ChatWidget::onSendPressed()
{
  if (sendButton->text() == _("Stop")) {
    aiService->cancelPendingRequests();
    if (activeAIBubble) {
      std::string stop_msg = activeResponseText ? *activeResponseText : "";
      stop_msg += "\n\n*[Request Stopped by User]*";
      activeAIBubble->updateText(QString::fromStdString(stop_msg));
      if (activeResponseText && !activeResponseText->empty()) {
        this->history.push_back({"assistant", *activeResponseText});
      }
    }
    isRequestRunning = false;
    activeAIBubble = nullptr;
    activeResponseText = nullptr;
    enableInput(true);
    return;
  }

  QString prompt = inputField->toPlainText().trimmed();
  if (prompt.isEmpty()) {
    return;
  }

  inputField->clear();
  addMessage(prompt, true);

  // Save to history
  history.push_back({"user", prompt.toStdString()});

  // Set active request states
  isRequestRunning = true;
  activeResponseText = std::make_shared<std::string>();
  activeAIBubble = addMessage(_("Thinking..."), false);

  // Disable input during streaming
  enableInput(false);

  auto alive = this->aliveState;

  aiService->chatCompletionStream(
    history,
    [this, alive](const std::string& chunk) {
      QMetaObject::invokeMethod(qApp, [this, alive, chunk]() {
        if (!*alive || !isRequestRunning) return;
        *activeResponseText += chunk;
        activeAIBubble->updateText(QString::fromStdString(*activeResponseText));
        // Auto-scroll to bottom
        this->scrollArea->verticalScrollBar()->setValue(
          this->scrollArea->verticalScrollBar()->maximum());
      });
    },
    [this, alive](const std::string& error_msg) {
      QMetaObject::invokeMethod(qApp, [this, alive, error_msg]() {
        if (!*alive || !isRequestRunning) return;
        std::string display_err = "Error: " + error_msg;
        if (activeResponseText->empty()) {
          activeAIBubble->updateText(QString::fromStdString(display_err));
        } else {
          this->addMessage(QString::fromStdString(display_err), false);
        }
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        this->enableInput(true);
      });
    },
    [this, alive]() {
      QMetaObject::invokeMethod(qApp, [this, alive]() {
        if (!*alive || !isRequestRunning) return;
        this->history.push_back({"assistant", *activeResponseText});
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        this->enableInput(true);
      });
    });
}

MessageBubble *ChatWidget::addMessage(const QString& text, bool isUser)
{
  MessageBubble *bubble = new MessageBubble(text, isUser, scrollAreaWidgetContents);
  scrollLayout->insertWidget(scrollLayout->count() - 1, bubble);

  // Auto-scroll to bottom after layout calculation
  QTimer::singleShot(50, this, [this]() {
    this->scrollArea->verticalScrollBar()->setValue(this->scrollArea->verticalScrollBar()->maximum());
  });

  return bubble;
}

void ChatWidget::onClearPressed()
{
  QLayoutItem *child;
  while (scrollLayout->count() > 1) {
    child = scrollLayout->takeAt(0);
    if (child->widget()) {
      delete child->widget();
    }
    delete child;
  }

  history.clear();
  activeToolBubble = nullptr;

  addMessage(_("Hello! I am your OpenSCAD AI assistant. Ask me to write some code, e.g. "
               "\"draw a sphere\" or \"create a box with a hole\"."),
             false);

  std::string defPrompt = aiService->getDefaultPrompt();
  if (!defPrompt.empty()) {
    inputField->setPlainText(QString::fromStdString(defPrompt));
  } else {
    inputField->clear();
  }
}

void ChatWidget::enableInput(bool enabled)
{
  inputField->setEnabled(enabled);
  clearButton->setEnabled(enabled);
  if (enabled) {
    sendButton->setText(_("Send"));
    inputField->setFocus();
    activeToolBubble = nullptr;
  } else {
    sendButton->setText(_("Stop"));
  }
}

bool ChatWidget::isDarkTheme() const
{
  QPalette pal = QApplication::palette();
  return pal.color(QPalette::Window).lightness() < 128;
}

void ChatWidget::proposeCodeChange(const std::string& code)
{
  proposedCode = code;

  // Retrieve current active editor code
  originalCode = "";
  MainWindow *mw = nullptr;
  for (auto *win : scadApp->windowManager.getWindows()) {
    mw = win;
    break;
  }
  if (mw && mw->activeEditor) {
    originalCode = mw->activeEditor->toPlainText().toStdString();
  }

  diffBannerWidget->show();
}

bool ChatWidget::hasPendingCodeChanges() const
{
  return diffBannerWidget && diffBannerWidget->isVisible();
}

void ChatWidget::logToolExecution(const std::string& name, const std::string& result)
{
  QString summary;
  QString detail;

  if (name == "get_editor_code") {
    summary = tr("Inspected current code");
    detail = tr("Tool: get_editor_code\nResult: Read %1 lines.")
               .arg(QString::fromStdString(result).count('\n'));
  } else if (name == "set_editor_code") {
    summary = tr("Proposed code changes");
    detail = tr("Tool: set_editor_code\nResult: Proposed changes to the active editor.");
  } else if (name == "trigger_preview") {
    summary = tr("Triggered render preview");
    detail = QString::fromStdString("Tool: trigger_preview\nResult: " + result);
  } else {
    summary = tr("Executed tool: %1").arg(QString::fromStdString(name));
    detail = QString::fromStdString("Tool: " + name + "\nResult: " + result);
  }

  // Find or create the active collapsible tool bubble
  if (!activeToolBubble || !isRequestRunning) {
    activeToolBubble = new CollapsibleBubble(summary, detail, this);
    scrollLayout->insertWidget(scrollLayout->count() - 1, activeToolBubble);
  } else {
    activeToolBubble->addToolCall(summary, detail);
  }

  // Scroll to bottom
  scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
}
