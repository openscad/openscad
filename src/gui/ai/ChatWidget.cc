#include "gui/ai/ChatWidget.h"
#include "gui/qtgettext.h"
#include "json/json.hpp"
#include <future>
#include <algorithm>
#include <cctype>
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
#include "gui/ai/ViewportGrabber.h"
#include "gui/QGLView.h"
#include "glview/Camera.h"
#include "gui/Console.h"
#include "gui/Preferences.h"

// MessageBubble implementation
MessageBubble::MessageBubble(const QString& text, bool isUser,
                             const std::vector<ImageAttachment>& images, QWidget *parent)
  : QWidget(parent)
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

  if (!images.empty()) {
    QHBoxLayout *imgLayout = new QHBoxLayout();
    imgLayout->setSpacing(6);
    for (const auto& img : images) {
      QByteArray data = QByteArray::fromBase64(QByteArray::fromStdString(img.base64_data));
      QImage qimg;
      if (qimg.loadFromData(data)) {
        QLabel *imgLabel = new QLabel(bubbleFrame);
        QPixmap pixmap =
          QPixmap::fromImage(qimg).scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imgLabel->setPixmap(pixmap);
        imgLabel->setStyleSheet("border-radius: 6px;");
        imgLayout->addWidget(imgLabel);
      }
    }
    imgLayout->addStretch(1);
    frameLayout->addLayout(imgLayout);
  }

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
  sendButton->setText(_("➜"));
  sendButton->setAccessibleName(_("Send"));
  sendButton->setStyleSheet(
    "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 18px; "
    "font-size: 15pt; font-weight: bold; } QPushButton:hover { background-color: #1d4ed8; }");
  inputField->setPlaceholderText(_("Ask anything about your OpenSCAD model…"));
  providerLabel->hide();
  providerBarWidget->hide();
  quickCommandsScrollArea->hide();
  providerComboBox->setToolTip(_("Choose a configured AI profile. Add API keys in AI Preferences."));
  // Keep the selector flexible so the composer still fits in a narrow dock.
  providerComboBox->setMinimumWidth(0);
  providerComboBox->setMaximumWidth(280);
  providerBarLayout->removeWidget(providerComboBox);
  providerComboBox->setParent(inputWidget);
  inputLayout->removeWidget(sendButton);
  sendButton->setParent(inputWidget);
  actionButtonsLayout->removeWidget(analyzeScreenButton);
  actionButtonsLayout->removeWidget(attachImageButton);
  // These actions are available from the composer's + menu. Hide the legacy
  // standalone buttons after removing them from the old toolbar layout.
  analyzeScreenButton->hide();
  attachImageButton->hide();
  agentModeCheckBox->hide();
  actionButtonsLayout->insertWidget(1, providerComboBox);
  actionButtonsLayout->addWidget(sendButton);

  // Connections
  connect(sendButton, &QPushButton::clicked, this, &ChatWidget::onSendPressed);
  connect(inputField, &ChatInputEdit::sendPressed, this, &ChatWidget::onSendPressed);
  connect(clearButton, &QPushButton::clicked, this, &ChatWidget::onClearPressed);
  connect(analyzeScreenButton, &QPushButton::clicked, this, &ChatWidget::onAnalyzeScreenPressed);
  connect(attachImageButton, &QPushButton::clicked, this, &ChatWidget::onAttachImagePressed);
  connect(agentModeCheckBox, &QCheckBox::toggled, this, [this](bool checked) { agenticMode = checked; });
  connect(providerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ChatWidget::onProviderChanged);

  // Setup chat options menu
  QMenu *chatMenu = new QMenu(this);
  chatMenu->addAction(_("Export Chat..."), this, &ChatWidget::exportChat);
  chatMenu->addAction(_("Import Chat..."), this, &ChatWidget::importChat);
  chatMenu->addSeparator();
  chatMenu->addAction(_("Copy as Markdown"), this, &ChatWidget::copyAsMarkdown);
  chatMenu->addSeparator();
  chatMenu->addAction(_("AI Preferences..."), this, []() {
    GlobalPreferences::inst()->show();
  });
  menuButton->setMenu(chatMenu);

  // Initialize backend and state
  aiService = std::make_shared<AIService>();
  aliveState = std::make_shared<bool>(true);

  // Populate provider profiles and select active profile
  updateProviderList();
  connect(GlobalPreferences::inst(), &Preferences::aiProfilesChanged,
          this, &ChatWidget::updateProviderList);

  // Initialize quick-command action pills above the chatbox
  setupQuickCommandsBar();

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

  // Register history drain callback: after each tool round, inject any pending
  // viewport snapshot into history so the model sees it on the next turn.
  aiService->registerHistoryDrainCallback([this](std::vector<ChatMessage>& hist) {
    QMetaObject::invokeMethod(
      qApp,
      [this, &hist]() {
        if (pendingViewportSnapshot.has_value()) {
          hist.push_back(std::move(*pendingViewportSnapshot));
          pendingViewportSnapshot.reset();
        }
      },
      Qt::DirectConnection);
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
        // In interactive mode, fire preview automatically after acceptance
        // (regardless of the dialog's own checkbox since the model already
        // called trigger_preview and it was deferred).
        if (pendingAutoPreview || dlg.shouldTriggerPreview()) {
          mw->actionRenderPreview();
        }
        pendingAutoPreview = false;
      }
      proposedCode = "";
      originalCode = "";
      diffBannerWidget->hide();
    } else if (result == 2) {  // Discarded Changes
      pendingAutoPreview = false;
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
  addMessage(prompt, true, pendingAttachments);

  // Save to history
  history.push_back({"user", prompt.toStdString(), "", "", pendingAttachments});

  pendingAttachments.clear();
  updateAttachmentPreviewBar();

  // Set active request states
  isRequestRunning = true;
  startNewResponseTurn();
  enableInput(false);
  agentModeCheckBox->setEnabled(false);

  MainWindow *mw = nullptr;
  for (auto *win : scadApp->windowManager.getWindows()) {
    mw = win;
    break;
  }
  const std::string currentCode = mw && mw->activeEditor
                                    ? mw->activeEditor->toPlainText().toStdString()
                                    : std::string{};
  auto alive = this->aliveState;
  aiService->generateOpenSCADCode(
    prompt.toStdString(), currentCode,
    [this, alive](const std::string& chunk) {
      QMetaObject::invokeMethod(qApp, [this, alive, chunk]() {
        if (!*alive || !isRequestRunning || !activeResponseText) return;
        *activeResponseText += chunk;
        if (activeAIBubble) activeAIBubble->updateText(QString::fromStdString(*activeResponseText));
        this->scrollArea->verticalScrollBar()->setValue(
          this->scrollArea->verticalScrollBar()->maximum());
      });
    },
    [this, alive](const std::string& error) {
      QMetaObject::invokeMethod(qApp, [this, alive, error]() {
        if (!*alive || !isRequestRunning) return;
        if (activeAIBubble) activeAIBubble->updateText(QString::fromStdString("Error: " + error));
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        enableInput(true);
        agentModeCheckBox->setEnabled(true);
      });
    },
    [this, alive]() {
      QMetaObject::invokeMethod(qApp, [this, alive]() {
        if (!*alive || !isRequestRunning) return;
        const std::string response = activeResponseText ? *activeResponseText : std::string{};
        std::string code = response;
        // Be tolerant of providers that wrap source in a Markdown code fence.
        const size_t fence = code.find("```");
        if (fence != std::string::npos) {
          const size_t lineEnd = code.find('\n', fence);
          const size_t closing = lineEnd == std::string::npos ? std::string::npos
                                                               : code.find("```", lineEnd + 1);
          if (lineEnd != std::string::npos && closing != std::string::npos)
            code = code.substr(lineEnd + 1, closing - lineEnd - 1);
        }
        while (!code.empty() && std::isspace(static_cast<unsigned char>(code.front()))) code.erase(code.begin());
        while (!code.empty() && std::isspace(static_cast<unsigned char>(code.back()))) code.pop_back();

        MainWindow *window = nullptr;
        for (auto *win : scadApp->windowManager.getWindows()) {
          window = win;
          break;
        }
        if (!window || !window->activeEditor) {
          activeAIBubble->updateText(tr("Code was generated, but there is no active editor tab. Open or create a .scad document and try again."));
          isRequestRunning = false;
          activeAIBubble = nullptr;
          activeResponseText = nullptr;
          enableInput(true);
          agentModeCheckBox->setEnabled(true);
          return;
        }
        if (code.empty() || code.find(';') == std::string::npos) {
          activeAIBubble->updateText(tr("The provider response did not look like OpenSCAD source, so the editor was left unchanged. Try a more specific model prompt."));
        } else {
          window->activeEditor->setText(QString::fromStdString(code));
          history.push_back({"assistant", code});
          activeAIBubble->updateText(tr("OpenSCAD code is in the editor. Click Render to see the model."));
        }
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        enableInput(true);
        agentModeCheckBox->setEnabled(true);
      });
    });
}

void ChatWidget::startAIStream(int retryCount)
{
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
    [this, alive, retryCount](const std::string& error_msg) {
      QMetaObject::invokeMethod(qApp, [this, alive, retryCount, error_msg]() {
        if (!*alive || !isRequestRunning) return;
        if (error_msg.find("HTTP status 503") != std::string::npos && retryCount < 2 &&
            activeResponseText && activeResponseText->empty()) {
          const int delayMs = 1500 * (1 << retryCount);
          activeAIBubble->updateText(
            QString(_("Gemini is busy. Retrying automatically (%1/2)…"))
              .arg(retryCount + 1));
          QTimer::singleShot(delayMs, this, [this, alive, retryCount]() {
            if (!*alive || !isRequestRunning) return;
            activeResponseText = std::make_shared<std::string>();
            if (activeAIBubble) activeAIBubble->updateText(_("Retrying Gemini request…"));
            startAIStream(retryCount + 1);
          });
          return;
        }
        std::string display_err = "Error: " + error_msg;
        if (error_msg.find("Connection refused") != std::string::npos) {
          display_err +=
            "\n\n*Troubleshooting Tip: Connection refused. Check your internet connection and verify "
            "that the configured AI provider endpoint is reachable.*";
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
        } else if (error_msg.find("HTTP status 503") != std::string::npos) {
          display_err +=
            "\n\n*Gemini is temporarily overloaded. The app retried automatically; please try again "
            "in a moment or select another configured model.*";
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
        this->agentModeCheckBox->setEnabled(true);
      });
    },
    [this, alive]() {
      QMetaObject::invokeMethod(qApp, [this, alive]() {
        if (!*alive || !isRequestRunning) return;
        if (activeResponseText && activeResponseText->empty()) {
          if (activeAIBubble) {
            // Keep an empty completion visible. Removing the thinking bubble
            // made failed or unsupported provider responses look like the send
            // button did nothing at all.
            activeAIBubble->updateText(
              tr("The AI provider returned an empty response. Check the provider/model settings and try again."));
          }
        } else if (activeResponseText) {
          this->history.push_back({"assistant", *activeResponseText});
          // Fallback: only in interactive mode — if the model wrote a code block in
          // chat text without calling set_editor_code, extract and propose it.
          // In agentic mode this never runs, preventing spurious diff dialogs.
          if (!agenticMode && !this->hasPendingCodeChanges()) {
            std::string text = *activeResponseText;
            size_t start = text.find("```");
            if (start != std::string::npos) {
              size_t code_start = text.find('\n', start);
              if (code_start != std::string::npos) {
                size_t end = text.find("```", code_start);
                if (end != std::string::npos) {
                  std::string code = text.substr(code_start + 1, end - (code_start + 1));
                  if (!code.empty()) {
                    this->proposeCodeChange(code);
                  }
                }
              }
            }
          }
        }
        isRequestRunning = false;
        activeAIBubble = nullptr;
        activeResponseText = nullptr;
        this->enableInput(true);
        this->agentModeCheckBox->setEnabled(true);
      });
    },
    !agenticMode);
}

void ChatWidget::startNewResponseTurn()
{
  activeResponseText = std::make_shared<std::string>();
  activeAIBubble = addMessage(_("Thinking..."), false);
  activeToolBubble = nullptr;
}

MessageBubble *ChatWidget::addMessage(const QString& text, bool isUser,
                                      const std::vector<ImageAttachment>& images)
{
  MessageBubble *bubble = new MessageBubble(text, isUser, images, scrollAreaWidgetContents);
  scrollLayout->insertWidget(scrollLayout->count() - 1, bubble);

  // Auto-scroll to bottom after layout calculation
  QTimer::singleShot(50, this, [this]() {
    this->scrollArea->verticalScrollBar()->setValue(this->scrollArea->verticalScrollBar()->maximum());
  });

  return bubble;
}

void ChatWidget::onAnalyzeScreenPressed()
{
  MainWindow *mw = nullptr;
  for (auto *win : scadApp->windowManager.getWindows()) {
    mw = win;
    break;
  }

  if (!mw || !mw->qglview) {
    QMessageBox::warning(this, _("Viewport Error"),
                         _("Could not find active 3D Viewport to grab screen."));
    return;
  }

  std::string base64_img = ViewportGrabber::captureViewportBase64(mw->qglview, 1024);
  if (base64_img.empty()) {
    QMessageBox::warning(this, _("Viewport Error"), _("Failed to capture 3D viewport frame."));
    return;
  }

  std::string meta = ViewportGrabber::serializeCameraMetadata(mw->qglview->cam);

  pendingAttachments.push_back({"image/png", base64_img});
  updateAttachmentPreviewBar();

  QString currentText = inputField->toPlainText();
  if (currentText.isEmpty()) {
    inputField->setPlainText(
      QString::fromStdString(meta + "\n\nDescribe what you see in this 3D view: "));
  } else if (!currentText.contains("[Viewport Camera Metadata]")) {
    inputField->setPlainText(QString::fromStdString(meta + "\n\n") + currentText);
  }
}

void ChatWidget::onAttachImagePressed()
{
  QString filePath = QFileDialog::getOpenFileName(this, _("Attach Image"), "",
                                                  _("Image Files (*.png *.jpg *.jpeg *.webp)"));

  if (filePath.isEmpty()) {
    return;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, _("File Error"), _("Could not open the selected image file."));
    return;
  }

  QByteArray data = file.readAll();
  std::string base64_str = data.toBase64().toStdString();

  std::string mime = "image/png";
  QString suffix = QFileInfo(filePath).suffix().toLower();
  if (suffix == "jpg" || suffix == "jpeg") {
    mime = "image/jpeg";
  } else if (suffix == "webp") {
    mime = "image/webp";
  }

  pendingAttachments.push_back({mime, base64_str});
  updateAttachmentPreviewBar();
}

void ChatWidget::removeAttachment(size_t index)
{
  if (index < pendingAttachments.size()) {
    pendingAttachments.erase(pendingAttachments.begin() + index);
    updateAttachmentPreviewBar();
  }
}

void ChatWidget::updateAttachmentPreviewBar()
{
  QLayoutItem *child;
  while ((child = attachmentPreviewLayout->takeAt(0)) != nullptr) {
    if (child->widget()) {
      delete child->widget();
    }
    delete child;
  }

  if (pendingAttachments.empty()) {
    attachmentPreviewWidget->setVisible(false);
    return;
  }

  attachmentPreviewWidget->setVisible(true);

  for (size_t i = 0; i < pendingAttachments.size(); ++i) {
    const auto& att = pendingAttachments[i];

    QWidget *itemWidget = new QWidget(attachmentPreviewWidget);
    QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(4, 2, 4, 2);
    itemLayout->setSpacing(4);

    QLabel *thumbLabel = new QLabel(itemWidget);
    QByteArray data = QByteArray::fromBase64(QByteArray::fromStdString(att.base64_data));
    QImage qimg;
    if (qimg.loadFromData(data)) {
      QPixmap pix =
        QPixmap::fromImage(qimg).scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      thumbLabel->setPixmap(pix);
    } else {
      thumbLabel->setText(_("[Img]"));
    }

    QPushButton *removeBtn = new QPushButton("X", itemWidget);
    removeBtn->setFixedSize(20, 20);
    removeBtn->setToolTip(_("Remove attachment"));
    connect(removeBtn, &QPushButton::clicked, this, [this, i]() { removeAttachment(i); });

    itemLayout->addWidget(thumbLabel);
    itemLayout->addWidget(removeBtn);

    itemWidget->setStyleSheet(
      "QWidget { background-color: #374151; border-radius: 6px; } QLabel { color: white; } QPushButton "
      "{ background-color: #ef4444; color: white; border: none; border-radius: 4px; font-weight: bold; "
      "}");

    attachmentPreviewLayout->addWidget(itemWidget);
  }

  attachmentPreviewLayout->addStretch(1);
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
    sendButton->setText(_("➜"));
    sendButton->setAccessibleName(_("Send"));
    sendButton->setToolTip(_("Send prompt"));
    inputField->setFocus();
    activeToolBubble = nullptr;
  } else {
    sendButton->setText(_("Stop"));
    sendButton->setAccessibleName(_("Stop"));
    sendButton->setToolTip(_("Stop generation"));
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
    summary = tr("Staged code proposal");
    detail = tr("Tool: set_editor_code\nResult: Code changes staged for manual review.");
  } else if (name == "trigger_preview") {
    summary = tr("Triggered render preview");
    detail = QString::fromStdString("Tool: trigger_preview\nResult: " + result);
  } else if (name == "get_viewport") {
    summary = tr("Inspected 3D Viewport");
    detail = QString::fromStdString("Tool: get_viewport\nResult: " + result);
  } else if (name == "set_camera") {
    summary = tr("Adjusted Camera View");
    detail = QString::fromStdString("Tool: set_camera\nResult: " + result);
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
      if (mw && mw->activeEditor) {
        if (agenticMode) {
          // In agent mode, put generated code in the active document so the next
          // trigger_preview call renders exactly what the AI just created.
          mw->activeEditor->setText(QString::fromStdString(code));
          result_val = "Success: Replaced the active editor contents with the generated OpenSCAD code. "
                       "Now call trigger_preview to compile and display the model.";
        } else {
          this->proposeCodeChange(code);
          result_val = "Success: Code change staged for manual review. The user will edit or accept it; "
                       "stop calling tools for this request.";
        }
      } else {
        result_val = "Error: No active editor found.";
      }
    }
  } else if (name == "trigger_preview") {
    if (hasPendingCodeChanges()) {
      // In interactive mode, the model called trigger_preview but code is
      // pending user review. Defer the preview and auto-fire it on acceptance.
      pendingAutoPreview = true;
      result_val =
        "Postponed: Code changes are pending user review. "
        "The preview will fire automatically once the user accepts the diff.";
    } else if (mw) {
      mw->actionRenderPreview();
      int new_errors = mw->compileErrors;
      int new_warnings = mw->compileWarnings;
      QString console_text = mw->console ? mw->console->toPlainText() : "";
      QStringList lines = console_text.split('\n', Qt::SkipEmptyParts);
      QString recent_console;
      int start_idx = std::max(0, static_cast<int>(lines.size()) - 15);
      for (int i = start_idx; i < lines.size(); ++i) {
        recent_console += lines[i] + "\n";
      }

      std::stringstream ss;
      if (new_errors > 0) {
        ss << "Error: Compilation failed with " << new_errors << " error(s) and " << new_warnings
           << " warning(s).\n";
        if (!recent_console.isEmpty()) {
          ss << "Recent console output:\n" << recent_console.toStdString();
        }
      } else if (new_warnings > 0) {
        ss << "Success with Warnings: Render completed with 0 errors and " << new_warnings
           << " warning(s).\n";
        if (!recent_console.isEmpty()) {
          ss << "Recent console output:\n" << recent_console.toStdString();
        }
      } else {
        ss << "Success: Render completed with 0 errors and 0 warnings.\n";
        if (!recent_console.isEmpty()) {
          ss << "Recent console output:\n" << recent_console.toStdString();
        }
      }
      result_val = ss.str();
      // After a successful render, capture the viewport and attach it to history
      // as a user-role message so the model can see the rendered result in the
      // next agentic turn. This avoids the pendingAttachments path which only
      // fires on explicit user sends.
      if (mw->qglview) {
        std::string img_b64 = ViewportGrabber::captureViewportBase64(mw->qglview, 512);
        if (!img_b64.empty()) {
          ChatMessage snap_msg;
          snap_msg.role = "user";
          snap_msg.content = "[Viewport snapshot after trigger_preview]";
          snap_msg.images.push_back({"image/png", img_b64});
          // This message is appended to history inside executeTool, which is
          // called synchronously from AIService's on_complete_wrapper before the
          // recursive chatCompletionStream call, so it will be included.
          // We return the image caption in result_val for tool message content.
          result_val += "\n(Viewport snapshot captured and attached for visual inspection.)";
          // Store for AIService to inject; we can't push to this->history directly
          // here because AIService owns the history vector passed by reference.
          // Instead, store it and append in the next on_complete_wrapper iteration.
          this->pendingViewportSnapshot = snap_msg;
        }
      }
    } else {
      result_val = "Error: No active MainWindow found.";
    }
  } else if (name == "get_viewport") {
    if (mw && mw->qglview) {
      const Camera& cam = mw->qglview->cam;
      std::string meta = ViewportGrabber::serializeCameraMetadata(cam);
      std::string img_b64 = ViewportGrabber::captureViewportBase64(mw->qglview, 1024);
      if (!img_b64.empty()) {
        if (agenticMode) {
          // In agentic mode: return image inline via pendingViewportSnapshot so
          // it reaches the model in the next turn rather than polluting the
          // user-send attachment queue.
          ChatMessage snap_msg;
          snap_msg.role = "user";
          snap_msg.content = "[Viewport snapshot from get_viewport]";
          snap_msg.images.push_back({"image/png", img_b64});
          this->pendingViewportSnapshot = snap_msg;
          meta += "\n(Viewport snapshot captured and will be included in the next model context.)";
        } else {
          // Interactive mode: attach to the pending attachments bar so the user
          // can see the image was captured and it goes with the next user send.
          this->pendingAttachments.push_back({"image/png", img_b64});
          this->updateAttachmentPreviewBar();
          meta += "\n(Captured viewport snapshot and attached to pending message context)";
        }
      }
      result_val = meta;
    } else {
      result_val = "Error: 3D viewport non-existent or inactive.";
    }
  } else if (name == "set_camera") {
    if (mw && mw->qglview) {
      try {
        auto args = nlohmann::json::parse(arguments_json);
        Camera cam = mw->qglview->cam;
        if (args.contains("vpt") && args["vpt"].is_array() && args["vpt"].size() == 3) {
          cam.setVpt(args["vpt"][0].get<double>(), args["vpt"][1].get<double>(),
                     args["vpt"][2].get<double>());
        }
        if (args.contains("vpr") && args["vpr"].is_array() && args["vpr"].size() == 3) {
          cam.setVpr(args["vpr"][0].get<double>(), args["vpr"][1].get<double>(),
                     args["vpr"][2].get<double>());
        }
        if (args.contains("vpd") && args["vpd"].is_number()) {
          cam.setVpd(args["vpd"].get<double>());
        }
        if (args.contains("vpf") && args["vpf"].is_number()) {
          cam.setVpf(args["vpf"].get<double>());
        }
        if (args.contains("projection") && args["projection"].is_string()) {
          std::string proj = args["projection"].get<std::string>();
          if (proj == "ortho" || proj == "orthogonal" || proj == "ORTHOGONAL") {
            cam.projection = Camera::ProjectionType::ORTHOGONAL;
          } else if (proj == "perspective" || proj == "PERSPECTIVE") {
            cam.projection = Camera::ProjectionType::PERSPECTIVE;
          }
        }
        mw->qglview->setCamera(cam);
        mw->qglview->update();
        result_val = "Success: Updated 3D viewport camera settings.\n" +
                     ViewportGrabber::serializeCameraMetadata(cam);
      } catch (const std::exception& e) {
        result_val = std::string("Error parsing set_camera arguments: ") + e.what();
      }
    } else {
      result_val = "Error: 3D viewport non-existent or inactive.";
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
    QFileDialog::getSaveFileName(this, _("Export Chat History"), "", _("JSON Files (*.json)"));

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
    m["tool_name"] = msg.tool_name;
    m["tool_calls"] = msg.tool_calls;

    if (!msg.images.empty()) {
      nlohmann::json img_arr = nlohmann::json::array();
      for (const auto& img : msg.images) {
        nlohmann::json im;
        im["mime_type"] = img.mime_type;
        im["base64_data"] = img.base64_data;
        img_arr.push_back(im);
      }
      m["images"] = img_arr;
    }

    hist_arr.push_back(m);
  }
  j["history"] = hist_arr;

  std::ofstream file(fileName.toStdString());
  if (!file.is_open()) {
    QMessageBox::warning(this, _("Export Warning"), _("Could not write to the chosen file."));
    return;
  }

  file << j.dump(2);
  QMessageBox::information(this, _("Export Successful"), _("Chat history exported successfully."));
}

void ChatWidget::importChat()
{
  QString fileName =
    QFileDialog::getOpenFileName(this, _("Import Chat History"), "", _("JSON Files (*.json)"));

  if (fileName.isEmpty()) {
    return;
  }

  std::ifstream file(fileName.toStdString());
  if (!file.is_open()) {
    QMessageBox::warning(this, _("Import Warning"), _("Could not open the selected file."));
    return;
  }

  nlohmann::json j;
  try {
    file >> j;
  } catch (const std::exception& e) {
    QMessageBox::warning(this, _("Import Warning"), tr("Failed to parse JSON file: %1").arg(e.what()));
    return;
  }

  if (!j.is_object() || !j.contains("history") || !j["history"].is_array()) {
    QMessageBox::warning(this, _("Import Warning"),
                         _("Selected file does not contain a valid chat history array."));
    return;
  }

  std::vector<ChatMessage> new_history;
  int skipped_count = 0;
  for (const auto& m : j["history"]) {
    if (!m.is_object()) {
      skipped_count++;
      continue;
    }
    ChatMessage msg;
    msg.role = m.value("role", "");
    msg.content = m.value("content", "");
    msg.tool_call_id = m.value("tool_call_id", "");
    msg.tool_name = m.value("tool_name", "");
    msg.tool_calls = m.value("tool_calls", "");

    if (m.contains("images") && m["images"].is_array()) {
      for (const auto& im : m["images"]) {
        if (im.is_object()) {
          ImageAttachment img;
          img.mime_type = im.value("mime_type", "image/png");
          img.base64_data = im.value("base64_data", "");
          if (!img.base64_data.empty()) {
            msg.images.push_back(img);
          }
        }
      }
    }

    if (!msg.role.empty()) {
      new_history.push_back(msg);
    } else {
      skipped_count++;
    }
  }

  this->history = std::move(new_history);
  rebuildChatUI();

  if (skipped_count > 0) {
    QMessageBox::warning(
      this, _("Import Warning"),
      tr("Imported chat history with warnings: %1 invalid entries were skipped.").arg(skipped_count));
  } else {
    QMessageBox::information(this, _("Import Successful"), _("Chat history imported successfully."));
  }
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
      addMessage(QString::fromStdString(msg.content), true, msg.images);
      activeToolBubble = nullptr;
    } else if (msg.role == "assistant") {
      if (!msg.content.empty() || !msg.images.empty()) {
        addMessage(QString::fromStdString(msg.content), false, msg.images);
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
                summary = tr("Applied code changes");
                detail = tr("Tool: set_editor_code\nResult: Applied code changes to the active editor.");
              } else if (name == "trigger_preview") {
                summary = tr("Triggered render preview");
                detail = QString::fromStdString("Tool: trigger_preview\nResult: " + result);
              } else if (name == "get_viewport") {
                summary = tr("Inspected 3D Viewport");
                detail = QString::fromStdString("Tool: get_viewport\nResult: " + result);
              } else if (name == "set_camera") {
                summary = tr("Adjusted Camera View");
                detail = QString::fromStdString("Tool: set_camera\nResult: " + result);
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

void ChatWidget::updateProviderList()
{
  isUpdatingProviders = true;
  providerComboBox->clear();

  std::vector<std::string> profiles = AIService::getConfiguredProfileNames();
  std::string activeProfile = AIService::getActiveProfileName();

  if (profiles.empty()) {
    providerComboBox->addItem(_("Set up an API in AI Preferences"), QString{});
    providerComboBox->setToolTip(_("No API keys are configured. Open the + menu and choose AI Preferences."));
    providerComboBox->setCurrentIndex(0);
    isUpdatingProviders = false;
    return;
  }

  int activeIndex = 0;
  for (size_t i = 0; i < profiles.size(); ++i) {
    const auto& name = profiles[i];
    QString displayName = QString::fromStdString(name);
    if (name == "Google Gemini") displayName = _("Gemini");
    else if (name == "OpenAI ChatGPT") displayName = _("ChatGPT");
    else if (name == "Anthropic Claude") displayName = _("Claude");
    providerComboBox->addItem(displayName, QString::fromStdString(name));
    if (name == activeProfile) {
      activeIndex = static_cast<int>(i);
    }
  }

  if (providerComboBox->count() > 0) {
    if (std::find(profiles.begin(), profiles.end(), activeProfile) == profiles.end()) {
      activeProfile = profiles.front();
      AIService::setActiveProfileName(activeProfile);
    }
    providerComboBox->setCurrentIndex(activeIndex);
  }
  isUpdatingProviders = false;
}

void ChatWidget::onProviderChanged(int index)
{
  if (isUpdatingProviders || index < 0) return;
  QString profileData = providerComboBox->itemData(index).toString();
  if (!profileData.isEmpty()) {
    AIService::setActiveProfileName(profileData.toStdString());
  }
}

void ChatWidget::setupQuickCommandsBar()
{
  struct CommandDef {
    QString label;
    QString prompt;
    QString tooltip;
    bool autoSend;
  };

  const std::vector<CommandDef> promptPills = {
    {_("✨ Create 3D Model..."),
     _("Create a detailed 3D model of "),
     _("Type any object to model (e.g. car, bracket, robot, phone stand)"),
     false},
    {_("🔧 Fix Syntax Errors"),
     _("Inspect the current code with get_editor_code and fix all syntax, compilation, and geometry errors"),
     _("Automatically diagnose and fix code errors"),
     true},
    {_("👁️ Render 3D Preview"),
     _("Compile and trigger a preview render of the current 3D model"),
     _("Trigger 3D viewport preview (F5)"),
     true},
    {_("🎨 Add Colors & Materials"),
     _("Add colorful realistic material styling to the current model using color() modifiers"),
     _("Colorize all parts of the 3D model"),
     true},
    {_("🎛️ Make Parametric"),
     _("Refactor this script to use customizer variables and parameters at the top so dimensions can be easily adjusted"),
     _("Turn dimensions into adjustable parameters"),
     true},
    {_("📐 Center at Origin"),
     _("Center the current 3D model around origin [0, 0, 0]"),
     _("Center model at origin"),
     true},
    {_("⭕ Smooth Curves ($fn)"),
     _("Improve surface smoothness and mesh resolution by setting appropriate $fn, $fa, and $fs values"),
     _("Smooth out curved surfaces"),
     true},
    {_("🔲 Round Sharp Edges"),
     _("Round the sharp corners and edges of the model using minkowski() or hull() fillets"),
     _("Add fillets/chamfers to sharp edges"),
     true},
    {_("🕳️ Add Mounting Holes"),
     _("Add screw mounting holes to the model using cylinder() difference()"),
     _("Cut screw/bolt mounting holes"),
     true},
    {_("🐚 Make Hollow Shell"),
     _("Hollow out the 3D model to create a shell with uniform wall thickness"),
     _("Create hollow interior with wall thickness"),
     true},
    {_("✂️ Cut / Split Model"),
     _("Create a clean cross-section cut of the model using difference() with a cutting cube"),
     _("Cut or split model in half"),
     true},
    {_("🧹 Clean & Optimize Code"),
     _("Clean up the code, organize it into clean reusable modules with comments, and optimize CSG performance"),
     _("Modularize and clean up code"),
     true},
    {_("📝 Explain Current Code"),
     _("Explain how the current OpenSCAD script is structured, what modules it defines, and what 3D geometry it builds"),
     _("Get a detailed explanation of the code"),
     true}
  };

  bool dark = isDarkTheme();
  QString pillStyle = dark
    ? "QPushButton {"
      "  background-color: #1e293b;"
      "  color: #93c5fd;"
      "  border: 1px solid #334155;"
      "  border-radius: 11px;"
      "  padding: 3px 10px;"
      "  font-size: 8.5pt;"
      "  font-weight: 500;"
      "}"
      "QPushButton:hover {"
      "  background-color: #2563eb;"
      "  color: #ffffff;"
      "  border-color: #3b82f6;"
      "}"
      "QPushButton:pressed {"
      "  background-color: #1d4ed8;"
      "}"
    : "QPushButton {"
      "  background-color: #f1f5f9;"
      "  color: #1e40af;"
      "  border: 1px solid #cbd5e1;"
      "  border-radius: 11px;"
      "  padding: 3px 10px;"
      "  font-size: 8.5pt;"
      "  font-weight: 500;"
      "}"
      "QPushButton:hover {"
      "  background-color: #3b82f6;"
      "  color: #ffffff;"
      "  border-color: #2563eb;"
      "}"
      "QPushButton:pressed {"
      "  background-color: #1d4ed8;"
      "}";

  QString menuBtnStyle = dark
    ? "QToolButton {"
      "  background-color: #0f172a;"
      "  color: #38bdf8;"
      "  border: 1px solid #0284c7;"
      "  border-radius: 11px;"
      "  padding: 3px 10px;"
      "  font-size: 8.5pt;"
      "  font-weight: bold;"
      "}"
      "QToolButton:hover {"
      "  background-color: #0284c7;"
      "  color: #ffffff;"
      "}"
    : "QToolButton {"
      "  background-color: #e0f2fe;"
      "  color: #0369a1;"
      "  border: 1px solid #0284c7;"
      "  border-radius: 11px;"
      "  padding: 3px 10px;"
      "  font-size: 8.5pt;"
      "  font-weight: bold;"
      "}"
      "QToolButton:hover {"
      "  background-color: #0284c7;"
      "  color: #ffffff;"
      "}";

  // Categorized Prompts Dropdown Menu
  QToolButton *promptsMenuBtn = new QToolButton(quickCommandsContent);
  promptsMenuBtn->setText(_("📋 All Prompts ▾"));
  promptsMenuBtn->setToolTip(_("Browse and select from all available AI prompt templates"));
  promptsMenuBtn->setPopupMode(QToolButton::InstantPopup);
  promptsMenuBtn->setStyleSheet(menuBtnStyle);
  promptsMenuBtn->setCursor(Qt::PointingHandCursor);

  QMenu *promptsMenu = new QMenu(promptsMenuBtn);

  // Group 1: 3D Generation Prompts
  QMenu *genMenu = promptsMenu->addMenu(_("✨ Create & Generate 3D Models"));
  genMenu->addAction(_("Create 3D Model of [object]..."), this, [this]() {
    this->onQuickCommandClicked(_("Create a detailed 3D model of "), false);
  });
  genMenu->addAction(_("Create Parametric Enclosure Box"), this, [this]() {
    this->onQuickCommandClicked(_("Create a 3D parametric electronics enclosure box with lid and screw holes"), true);
  });
  genMenu->addAction(_("Create Functional Mechanical Bracket"), this, [this]() {
    this->onQuickCommandClicked(_("Create a 3D functional mechanical mounting bracket with counterbored holes"), true);
  });

  // Group 2: Code Quality & Fixes
  QMenu *fixMenu = promptsMenu->addMenu(_("🔧 Code Fixes & Optimization"));
  fixMenu->addAction(_("Fix Syntax & Compilation Errors"), this, [this]() {
    this->onQuickCommandClicked(_("Inspect the current code with get_editor_code and fix all syntax, compilation, and geometry errors"), true);
  });
  fixMenu->addAction(_("Clean Up & Modularize Code"), this, [this]() {
    this->onQuickCommandClicked(_("Clean up the code, organize it into clean reusable modules with comments, and optimize CSG performance"), true);
  });
  fixMenu->addAction(_("Explain Current Code"), this, [this]() {
    this->onQuickCommandClicked(_("Explain how the current OpenSCAD script is structured, what modules it defines, and what 3D geometry it builds"), true);
  });

  // Group 3: Geometry & Dimensions
  QMenu *geomMenu = promptsMenu->addMenu(_("📐 Geometry & Dimensions"));
  geomMenu->addAction(_("Center Model at Origin [0, 0, 0]"), this, [this]() {
    this->onQuickCommandClicked(_("Center the current 3D model around origin [0, 0, 0]"), true);
  });
  geomMenu->addAction(_("Smooth Curved Surfaces ($fn)"), this, [this]() {
    this->onQuickCommandClicked(_("Improve surface smoothness and mesh resolution by setting appropriate $fn, $fa, and $fs values"), true);
  });
  geomMenu->addAction(_("Round Sharp Edges (Fillets)"), this, [this]() {
    this->onQuickCommandClicked(_("Round the sharp corners and edges of the model using minkowski() or hull() fillets"), true);
  });
  geomMenu->addAction(_("Add Screw Mounting Holes"), this, [this]() {
    this->onQuickCommandClicked(_("Add screw mounting holes to the model using cylinder() difference()"), true);
  });
  geomMenu->addAction(_("Make Hollow Shell"), this, [this]() {
    this->onQuickCommandClicked(_("Hollow out the 3D model to create a shell with uniform wall thickness"), true);
  });
  geomMenu->addAction(_("Cut / Split Model"), this, [this]() {
    this->onQuickCommandClicked(_("Create a clean cross-section cut of the model using difference() with a cutting cube"), true);
  });

  // Group 4: Appearance & Rendering
  QMenu *renderMenu = promptsMenu->addMenu(_("🎨 Appearance & Preview"));
  renderMenu->addAction(_("Add Colors & Materials"), this, [this]() {
    this->onQuickCommandClicked(_("Add colorful realistic material styling to the current model using color() modifiers"), true);
  });
  renderMenu->addAction(_("Make Model Parametric (Customizer)"), this, [this]() {
    this->onQuickCommandClicked(_("Refactor this script to use customizer variables and parameters at the top so dimensions can be easily adjusted"), true);
  });
  renderMenu->addAction(_("Render 3D Viewport Preview"), this, [this]() {
    this->onQuickCommandClicked(_("Compile and trigger a preview render of the current 3D model"), true);
  });

  promptsMenu->addSeparator();
  promptsMenu->addAction(_("Analyze OpenSCAD viewport"), this, &ChatWidget::onAnalyzeScreenPressed);
  promptsMenu->addAction(_("Attach image…"), this, &ChatWidget::onAttachImagePressed);
  promptsMenu->addAction(_("Refresh configured profiles"), this, &ChatWidget::updateProviderList);
  promptsMenu->addAction(_("AI Preferences…"), this, []() { GlobalPreferences::inst()->show(); });
  QAction *agentModeAction = promptsMenu->addAction(_("Agent mode"));
  agentModeAction->setCheckable(true);
  agentModeAction->setChecked(agentModeCheckBox->isChecked());
  connect(agentModeAction, &QAction::toggled, agentModeCheckBox, &QCheckBox::setChecked);

  promptsMenuBtn->setMenu(promptsMenu);
  quickCommandsLayout->addWidget(promptsMenuBtn);
  composerActionsButton->setText(_("＋"));
  composerActionsButton->setMenu(promptsMenu);

  // Horizontal Quick-Prompt Pills
  // The full set is available from the categorized menu. Keep only the
  // primary model creation shortcut inline so narrow docks remain tidy.
  bool primaryPromptAdded = false;
  for (const auto& cmd : promptPills) {
    if (primaryPromptAdded) break;
    primaryPromptAdded = true;
    QPushButton *btn = new QPushButton(cmd.label, quickCommandsContent);
    btn->setToolTip(cmd.tooltip);
    btn->setStyleSheet(pillStyle);
    btn->setCursor(Qt::PointingHandCursor);

    QString promptText = cmd.prompt;
    bool autoSend = cmd.autoSend;
    connect(btn, &QPushButton::clicked, this, [this, promptText, autoSend]() {
      this->onQuickCommandClicked(promptText, autoSend);
    });

    quickCommandsLayout->addWidget(btn);
  }
}

void ChatWidget::onQuickCommandClicked(const QString& promptText, bool autoSend)
{
  if (isRequestRunning) return;
  inputField->setPlainText(promptText);
  if (autoSend) {
    onSendPressed();
  } else {
    inputField->setFocus();
    QTextCursor cursor = inputField->textCursor();
    cursor.movePosition(QTextCursor::End);
    inputField->setTextCursor(cursor);
  }
}
