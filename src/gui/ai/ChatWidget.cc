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

  QLabel *label = new QLabel(text, bubbleFrame);
  label->setWordWrap(true);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  label->setStyleSheet(labelStyle);

  frameLayout->addWidget(label);
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

  // Initial welcome greeting
  addMessage(_("Hello! I am your OpenSCAD AI assistant. Note: AI connection is under development and is "
               "not yet active. I currently only simulate responses.\n\nAsk me to write some code, e.g. "
               "\"draw a sphere\" or \"create a box with a hole\"."),
             false);
}

ChatWidget::~ChatWidget()
{
}

void ChatWidget::onSendPressed()
{
  QString prompt = inputField->toPlainText().trimmed();
  if (prompt.isEmpty()) {
    return;
  }

  inputField->clear();
  addMessage(prompt, true);

  // Add a mock thinking bubble
  QWidget *thinkingBubble = addMessage(_("AI is thinking..."), false);

  // Schedule a simulated response after 1 second
  QTimer::singleShot(
    1000, this, [this, prompt, thinkingBubble]() { this->simulateAIResponse(prompt, thinkingBubble); });
}

void ChatWidget::simulateAIResponse(const QString& prompt, QWidget *thinkingBubble)
{
  if (thinkingBubble) {
    scrollLayout->removeWidget(thinkingBubble);
    delete thinkingBubble;
  }

  QString reply;
  QString lowercasePrompt = prompt.toLower();

  if (lowercasePrompt.contains("sphere")) {
    reply =
      _("Here is how you can create a sphere in OpenSCAD:\n\n```openscad\n// Sphere with radius 10 and "
        "smooth details\nsphere(r = 10, $fn = 100);\n```");
  } else if (lowercasePrompt.contains("cube") || lowercasePrompt.contains("box")) {
    reply =
      _("Here is how you can create a cube in OpenSCAD:\n\n```openscad\n// Cube centered at the "
        "origin\ncube(size = [10, 20, 15], center = true);\n```");
  } else if (lowercasePrompt.contains("cylinder")) {
    reply =
      _("Here is how you can create a cylinder in OpenSCAD:\n\n```openscad\n// Cylinder with height 20 "
        "and radius 5\ncylinder(h = 20, r = 5, center = true, $fn = 50);\n```");
  } else {
    reply = QString(_("I received your prompt: \"%1\".\n\nI can help you build 3D models using "
                      "OpenSCAD! Try asking me to create a shape like a 'sphere' or a 'cube'."))
              .arg(prompt);
  }

  addMessage(reply, false);
}

QWidget *ChatWidget::addMessage(const QString& text, bool isUser)
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

  addMessage(_("Hello! I am your OpenSCAD AI assistant. Note: AI connection is under development and is "
               "not yet active. I currently only simulate responses.\n\nAsk me to write some code, e.g. "
               "\"draw a sphere\" or \"create a box with a hole\"."),
             false);
}

bool ChatWidget::isDarkTheme() const
{
  QPalette pal = QApplication::palette();
  return pal.color(QPalette::Window).lightness() < 128;
}
