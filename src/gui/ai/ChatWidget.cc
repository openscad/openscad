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
}

ChatWidget::~ChatWidget()
{
  *aliveState = false;
}

void ChatWidget::onSendPressed()
{
  QString prompt = inputField->toPlainText().trimmed();
  if (prompt.isEmpty()) {
    return;
  }

  inputField->clear();
  addMessage(prompt, true);

  // Save to history
  history.push_back({"user", prompt.toStdString()});

  // Add the AI message bubble initialized to "Thinking..."
  MessageBubble *aiBubble = addMessage(_("Thinking..."), false);

  // Disable input during streaming
  enableInput(false);

  auto alive = this->aliveState;
  auto responseText = std::make_shared<std::string>();

  aiService->chatCompletionStream(
    history,
    [this, alive, responseText, aiBubble](const std::string& chunk) {
      QMetaObject::invokeMethod(qApp, [this, alive, responseText, aiBubble, chunk]() {
        if (!*alive) return;
        *responseText += chunk;
        aiBubble->updateText(QString::fromStdString(*responseText));
        // Auto-scroll to bottom
        this->scrollArea->verticalScrollBar()->setValue(
          this->scrollArea->verticalScrollBar()->maximum());
      });
    },
    [this, alive, responseText, aiBubble](const std::string& error_msg) {
      QMetaObject::invokeMethod(qApp, [this, alive, responseText, aiBubble, error_msg]() {
        if (!*alive) return;
        std::string display_err = "Error: " + error_msg;
        if (responseText->empty()) {
          aiBubble->updateText(QString::fromStdString(display_err));
        } else {
          this->addMessage(QString::fromStdString(display_err), false);
        }
        this->enableInput(true);
      });
    },
    [this, alive, responseText]() {
      QMetaObject::invokeMethod(qApp, [this, alive, responseText]() {
        if (!*alive) return;
        this->history.push_back({"assistant", *responseText});
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
  sendButton->setEnabled(enabled);
  clearButton->setEnabled(enabled);
  if (enabled) {
    inputField->setFocus();
  }
}

bool ChatWidget::isDarkTheme() const
{
  QPalette pal = QApplication::palette();
  return pal.color(QPalette::Window).lightness() < 128;
}
