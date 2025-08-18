#include "ChatWidget.h"
#include "core/Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    setupUI();
    
    // Load API key from settings
    apiKey = QString::fromStdString(Settings::Settings::openaiApiKey.value());
    apiUrl = "https://api.openai.com/v1/chat/completions";
    
    if (apiKey.isEmpty()) {
        addMessage("⚠️ OpenAI API key not set. Go to Edit → Preferences to configure.", false);
    }
}

void ChatWidget::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // Chat messages area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    messagesWidget = new QWidget();
    messagesLayout = new QVBoxLayout(messagesWidget);
    messagesLayout->setContentsMargins(5, 5, 5, 5);
    messagesLayout->setSpacing(10);
    messagesLayout->addStretch();
    
    scrollArea->setWidget(messagesWidget);
    mainLayout->addWidget(scrollArea);
    
    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Ask GPT about OpenSCAD...");
    
    sendButton = new QPushButton("Send", this);
    sendButton->setMaximumWidth(60);
    
    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);
    mainLayout->addLayout(inputLayout);
    
    // Connect signals
    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(inputField, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
    
    // Welcome message
    addMessage("💬 Welcome to OpenSCAD Chat! Ask me anything about 3D modeling, OpenSCAD syntax, or get help with your designs.", false);
}

void ChatWidget::sendMessage()
{
    QString message = inputField->text().trimmed();
    if (message.isEmpty()) return;
    
    inputField->clear();
    addMessage(message, true);
    
    if (apiKey.isEmpty()) {
        addMessage("❌ Please set your OpenAI API key in preferences first.", false);
        return;
    }
    
    // Add user message to conversation history
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = message;
    conversationHistory.append(userMessage);
    
    // Prepare API request
    QJsonObject requestData;
    requestData["model"] = "gpt-3.5-turbo";
    requestData["messages"] = conversationHistory;
    requestData["max_tokens"] = 500;
    requestData["temperature"] = 0.7;
    
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson();
    
    QUrl url("https://api.openai.com/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    QNetworkReply *reply = networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, &ChatWidget::onApiResponse);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &ChatWidget::onApiError);
    
    // Show loading message
    addMessage("🤔 Thinking...", false);
    sendButton->setEnabled(false);
}

void ChatWidget::onApiResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    sendButton->setEnabled(true);
    
    // Remove "Thinking..." message
    if (messagesLayout->count() > 1) {
        QLayoutItem *item = messagesLayout->takeAt(messagesLayout->count() - 2);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError) {
            QJsonObject obj = doc.object();
            QJsonArray choices = obj["choices"].toArray();
            
            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                QJsonObject message = choice["message"].toObject();
                QString content = message["content"].toString();
                
                addMessage(content, false);
                
                // Add assistant response to conversation history
                QJsonObject assistantMessage;
                assistantMessage["role"] = "assistant";
                assistantMessage["content"] = content;
                conversationHistory.append(assistantMessage);
            } else {
                addMessage("❌ No response from API", false);
            }
        } else {
            addMessage("❌ Failed to parse API response", false);
        }
    } else {
        addMessage(QString("❌ API Error: %1").arg(reply->errorString()), false);
    }
    
    reply->deleteLater();
}

void ChatWidget::onApiError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    sendButton->setEnabled(true);
    
    // Remove "Thinking..." message
    if (messagesLayout->count() > 1) {
        QLayoutItem *item = messagesLayout->takeAt(messagesLayout->count() - 2);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    addMessage("❌ Network error occurred", false);
}

void ChatWidget::addMessage(const QString& message, bool isUser)
{
    QLabel *messageLabel = new QLabel(formatMessage(message, isUser));
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    messageLabel->setMargin(8);
    
    if (isUser) {
        messageLabel->setStyleSheet(
            "QLabel { "
            "background-color: #007acc; "
            "color: white; "
            "border-radius: 10px; "
            "padding: 8px; "
            "margin-left: 50px; "
            "}"
        );
        messageLabel->setAlignment(Qt::AlignRight);
    } else {
        messageLabel->setStyleSheet(
            "QLabel { "
            "background-color: #f0f0f0; "
            "color: black; "
            "border-radius: 10px; "
            "padding: 8px; "
            "margin-right: 50px; "
            "}"
        );
        messageLabel->setAlignment(Qt::AlignLeft);
    }
    
    // Insert before the stretch
    messagesLayout->insertWidget(messagesLayout->count() - 1, messageLabel);
    scrollToBottom();
}

void ChatWidget::scrollToBottom()
{
    QApplication::processEvents();
    scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
}

QString ChatWidget::formatMessage(const QString& text, bool isUser)
{
    QString prefix = isUser ? "You: " : "GPT: ";
    return prefix + text;
}
