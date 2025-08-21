#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollArea>
#include <QLabel>

class ChatWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ChatWidget(QWidget *parent = nullptr);
  ~ChatWidget() override = default;

private slots:
  void sendMessage();
  void onApiResponse();
  void onApiError(QNetworkReply::NetworkError error);

private:
  void setupUI();
  void addMessage(const QString& message, bool isUser);
  void scrollToBottom();
  QString formatMessage(const QString& text, bool isUser);

  QVBoxLayout *mainLayout;
  QScrollArea *scrollArea;
  QWidget *messagesWidget;
  QVBoxLayout *messagesLayout;
  QLineEdit *inputField;
  QPushButton *sendButton;
  QNetworkAccessManager *networkManager;

  QJsonArray conversationHistory;
};
