#include "gui/ai/ChatWidget.h"
#include "gui/qtgettext.h"
#include "json/json.hpp"
#include <future>
#include <QScrollBar>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QPalette>
#include <QMenu>
#include <QToolButton>
#include <QFileDialog>
#include <QClipboard>
#include <QDateTime>
#include <QMessageBox>
#include <fstream>
#include "json/json.hpp"
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

  // Setup chat options menu
  QMenu *chatMenu = new QMenu(this);
  chatMenu->addAction(_("Export Chat..."), this, &ChatWidget::exportChat);
  chatMenu->addAction(_("Import Chat..."), this, &ChatWidget::importChat);
  chatMenu->addSeparator();
  chatMenu->addAction(_("Copy as Markdown"), this, &ChatWidget::copyAsMarkdown);
  menuButton->setMenu(chatMenu);

  // Initialize backend and state
  aiService = std::make_shared<AIService>();
  aliveState = std::make_shared<bool>(true);

  // Register tool executor callback
  aiService->registerToolExecutor([this](const std::string& name, const std::string& arguments_json) {
    auto promise = std::make_shared<std::promise<std::string>>();
    auto future = promise->get_future();

    QMetaObject::invokeMethod(
      qApp,
      [this, promise, name, arguments_json]() {
        try {
          std::string result_val = this->executeTool(name, arguments_json);
          promise->set_value(result_val);
        } catch (const std::exception& e) {
          promise->set_value(std::string("Error parsing/executing tool: ") + e.what());
        } catch (...) {
          promise->set_value("Error: Unknown exception occurred during tool execution.");
        }
      },
      Qt::QueuedConnection);

    return future.get();
  });

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
      if (stop_msg.empty()) {
        scrollLayout->removeWidget(activeAIBubble);
        delete activeAIBubble;
      } else {
        stop_msg += "\n\n*[Request Stopped by User]*";
        activeAIBubble->updateText(QString::fromStdString(stop_msg));
        if (activeResponseText && !activeResponseText->empty()) {
          this->history.push_back({"assistant", *activeResponseText});
        }
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

  if (prompt.length() > 100000) {
    addMessage(prompt, true);
    addMessage(
      tr("Error: Prompt size exceeds the limit of 100,000 characters. Please shorten your prompt."),
      false);
    return;
  }

  inputField->clear();
  addMessage(prompt, true);

  // Save to history
  history.push_back({"user", prompt.toStdString()});

  // Set active request states
  isRequestRunning = true;
  startNewResponseTurn();

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
        if (error_msg.find("Connection refused") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: Connection refused. Please check if your local model server "
            "(such as Ollama or LM Studio) is running and listening on the configured port.*";
        } else if (error_msg.find("Host not found") != std::string::npos ||
                   error_msg.find("unreachable") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: Host unreachable. Please check your internet connection and "
            "verify that the API endpoint URL in Preferences is correct.*";
        } else if (error_msg.find("timed out") != std::string::npos ||
                   error_msg.find("Timeout") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: The request timed out. The server might be busy or offline. "
            "Please try again.*";
        } else if (error_msg.find("HTTP status 401") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: Unauthorized. Please check if your API Key is entered correctly "
            "in Preferences -> AI tab.*";
        } else if (error_msg.find("HTTP status 404") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: Not Found. Please check if the endpoint URL and model name are "
            "configured correctly in Preferences.*";
        }
        if (activeResponseText && activeResponseText->empty()) {
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
        if (activeResponseText && activeResponseText->empty()) {
          if (activeAIBubble) {
            scrollLayout->removeWidget(activeAIBubble);
            delete activeAIBubble;
            activeAIBubble = nullptr;
          }
        } else if (activeResponseText) {
          this->history.push_back({"assistant", *activeResponseText});
        }
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        this->enableInput(true);
      });
    });
}

void ChatWidget::startNewResponseTurn()
{
  activeResponseText = std::make_shared<std::string>();
  activeAIBubble = addMessage(_("Thinking..."), false);
  activeToolBubble = nullptr;
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
    int idx = scrollLayout->indexOf(activeAIBubble);
    if (idx != -1) {
      scrollLayout->insertWidget(idx, activeToolBubble);
    } else {
      scrollLayout->insertWidget(scrollLayout->count() - 1, activeToolBubble);
    }
  } else {
    activeToolBubble->addToolCall(summary, detail);
  }

  // Scroll to bottom
  scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
}

std::string ChatWidget::executeTool(const std::string& name, const std::string& arguments_json)
{
  MainWindow *mw = nullptr;
  for (auto *win : scadApp->windowManager.getWindows()) {
    mw = win;
    break;
  }

  std::string result_val;
  int limit = aiService ? aiService->getPayloadLimit() : 50000;

  if (name == "get_editor_code") {
    if (mw && mw->activeEditor) {
      std::string code = mw->activeEditor->toPlainText().toStdString();
      if (static_cast<int>(code.size()) > limit) {
        result_val = "Error: The active editor script is too large (" + std::to_string(code.size()) +
                     " bytes). The maximum allowed size for AI analysis is " + std::to_string(limit) +
                     " bytes. Please reduce the script size.";
      } else {
        result_val = code;
      }
    } else {
      result_val = "Error: No active editor found.";
    }
  } else if (name == "set_editor_code") {
    auto args = nlohmann::json::parse(arguments_json);
    if (!args.contains("code")) {
      return "Error: Missing required argument 'code'.";
    }
    std::string code = args["code"].get<std::string>();
    if (static_cast<int>(code.size()) > limit) {
      result_val = "Error: Proposed code change is too large (" + std::to_string(code.size()) +
                   " bytes). The maximum allowed size is " + std::to_string(limit) + " bytes.";
    } else {
      this->proposeCodeChange(code);
      result_val =
        "Success: Code change proposed to the user for review. The user will review and choose whether "
        "to "
        "apply it.";
    }
  } else if (name == "trigger_preview") {
    if (this->hasPendingCodeChanges()) {
      result_val = "Info: Preview postponed because code changes are pending user review.";
    } else {
      if (mw) {
        mw->actionRenderPreview();
        result_val = "Success: Preview triggered.";
      } else {
        result_val = "Error: No active MainWindow found.";
      }
    }
  } else {
    result_val = "Error: Unknown tool name '" + name + "'.";
  }

  this->logToolExecution(name, result_val);
  return result_val;
}

void ChatWidget::exportChat()
{
  QString fileName =
    QFileDialog::getSaveFileName(this, _("Export Chat History"), "", _("OpenSCAD Chat (*.oschat.json)"));

  if (fileName.isEmpty()) {
    return;
  }

  nlohmann::json j;
  j["version"] = 1;
  j["application"] = "OpenSCAD";
  j["exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();

  nlohmann::json hist_arr = nlohmann::json::array();
  for (const auto& msg : this->history) {
    nlohmann::json m;
    m["role"] = msg.role;
    m["content"] = msg.content;
    m["tool_call_id"] = msg.tool_call_id;
    m["tool_calls"] = msg.tool_calls;
    hist_arr.push_back(m);
  }
  j["history"] = hist_arr;

  std::ofstream file(fileName.toStdString());
  if (!file.is_open()) {
    QMessageBox::critical(this, _("Export Error"), _("Could not write to the chosen file."));
    return;
  }

  file << j.dump(2);
  QMessageBox::information(this, _("Export Successful"), _("Chat history exported successfully."));
}

void ChatWidget::importChat()
{
  QString fileName =
    QFileDialog::getOpenFileName(this, _("Import Chat History"), "", _("OpenSCAD Chat (*.oschat.json)"));

  if (fileName.isEmpty()) {
    return;
  }

  std::ifstream file(fileName.toStdString());
  if (!file.is_open()) {
    QMessageBox::critical(this, _("Import Error"), _("Could not open the chosen file."));
    return;
  }

  nlohmann::json j;
  try {
    file >> j;
  } catch (const std::exception& e) {
    QMessageBox::critical(this, _("Import Error"), tr("Failed to parse JSON file: %1").arg(e.what()));
    return;
  }

  if (!j.is_object() || !j.contains("history") || !j["history"].is_array()) {
    QMessageBox::critical(this, _("Import Error"), _("Invalid chat file: missing history array."));
    return;
  }

  std::vector<ChatMessage> new_history;
  for (const auto& m : j["history"]) {
    if (!m.is_object()) continue;
    ChatMessage msg;
    msg.role = m.value("role", "");
    msg.content = m.value("content", "");
    msg.tool_call_id = m.value("tool_call_id", "");
    msg.tool_calls = m.value("tool_calls", "");
    new_history.push_back(msg);
  }

  this->history = std::move(new_history);
  rebuildChatUI();

  QMessageBox::information(this, _("Import Successful"), _("Chat history imported successfully."));
}

void ChatWidget::copyAsMarkdown()
{
  std::string markdown = "";
  for (const auto& msg : this->history) {
    if (msg.role == "system") {
      continue;
    } else if (msg.role == "user") {
      markdown += "**User**:\n" + msg.content + "\n\n";
    } else if (msg.role == "assistant") {
      if (!msg.content.empty()) {
        markdown += "**AI**:\n" + msg.content + "\n\n";
      }
      if (!msg.tool_calls.empty()) {
        try {
          auto tcs_json = nlohmann::json::parse(msg.tool_calls);
          if (tcs_json.is_array()) {
            for (auto& tc_json : tcs_json) {
              std::string name = "";
              if (tc_json.contains("function") && tc_json["function"].is_object()) {
                name = tc_json["function"].value("name", "");
              }
              markdown += "*[Executed Tool: " + name + "]*\n\n";
            }
          }
        } catch (...) {
        }
      }
    }
  }

  QApplication::clipboard()->setText(QString::fromStdString(markdown));
  QMessageBox::information(this, _("Chat Copied"),
                           _("Chat conversation copied to clipboard as Markdown."));
}

void ChatWidget::rebuildChatUI()
{
  // First, clear everything in scrollLayout except the last item (scrollSpacer)
  QLayoutItem *child;
  while (scrollLayout->count() > 1) {
    child = scrollLayout->takeAt(0);
    if (child->widget()) {
      delete child->widget();
    }
    delete child;
  }
  activeToolBubble = nullptr;

  // Replay history to build bubbles
  for (size_t i = 0; i < this->history.size(); ++i) {
    const auto& msg = this->history[i];
    if (msg.role == "system") {
      continue;
    } else if (msg.role == "user") {
      addMessage(QString::fromStdString(msg.content), true);
      activeToolBubble = nullptr;
    } else if (msg.role == "assistant") {
      if (!msg.content.empty()) {
        addMessage(QString::fromStdString(msg.content), false);
        activeToolBubble = nullptr;
      }
      if (!msg.tool_calls.empty()) {
        try {
          auto tcs_json = nlohmann::json::parse(msg.tool_calls);
          if (tcs_json.is_array()) {
            for (auto& tc_json : tcs_json) {
              std::string id = tc_json.value("id", "");
              std::string name = "";
              if (tc_json.contains("function") && tc_json["function"].is_object()) {
                name = tc_json["function"].value("name", "");
              }

              // Find corresponding tool result
              std::string result = "No response";
              for (size_t j = i + 1; j < this->history.size(); ++j) {
                if (this->history[j].role == "tool" && this->history[j].tool_call_id == id) {
                  result = this->history[j].content;
                  break;
                }
              }

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

              if (!activeToolBubble) {
                activeToolBubble = new CollapsibleBubble(summary, detail, this);
                scrollLayout->insertWidget(scrollLayout->count() - 1, activeToolBubble);
              } else {
                activeToolBubble->addToolCall(summary, detail);
              }
            }
          }
        } catch (...) {
        }
      }
    }
  }
}
