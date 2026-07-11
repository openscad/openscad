#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QPalette>
#include <QApplication>
#include <vector>
#include <QString>

class CollapsibleBubble : public QWidget
{
  Q_OBJECT
public:
  CollapsibleBubble(const QString& summary, const QString& detail, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);

    QFrame *bubbleFrame = new QFrame(this);
    bubbleFrame->setFrameShape(QFrame::StyledPanel);

    bool dark = isDarkTheme();
    QString frameStyle =
      dark ? "QFrame { background-color: #1e293b; border: 1px solid #334155; border-radius: 8px; }"
           : "QFrame { background-color: #f3f4f6; border: 1px solid #e5e7eb; border-radius: 8px; }";
    bubbleFrame->setStyleSheet(frameStyle);

    QVBoxLayout *frameLayout = new QVBoxLayout(bubbleFrame);
    frameLayout->setContentsMargins(8, 8, 8, 8);

    toggleButton = new QPushButton(this);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false);
    toggleButton->setFlat(true);
    toggleButton->setStyleSheet(dark ? "QPushButton { text-align: left; font-weight: bold; color: "
                                       "#9ca3af; padding: 0; border: none; font-size: 9pt; }"
                                       "QPushButton:hover { color: #f3f4f6; }"
                                     : "QPushButton { text-align: left; font-weight: bold; color: "
                                       "#4b5563; padding: 0; border: none; font-size: 9pt; }"
                                       "QPushButton:hover { color: #1f2937; }");

    detailsLabel = new QLabel(this);
    detailsLabel->setWordWrap(true);
    detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailsLabel->setStyleSheet(
      dark ? "QLabel { color: #d1d5db; padding-top: 6px; font-size: 9pt; font-family: monospace; }"
           : "QLabel { color: #4b5563; padding-top: 6px; font-size: 9pt; font-family: monospace; }");
    detailsLabel->hide();

    frameLayout->addWidget(toggleButton);
    frameLayout->addWidget(detailsLabel);
    layout->addWidget(bubbleFrame);

    addToolCall(summary, detail);

    connect(toggleButton, &QPushButton::clicked, this, [this](bool checked) {
      detailsLabel->setVisible(checked);
      updateButtonText();
    });
  }

  void addToolCall(const QString& summary, const QString& detail)
  {
    toolCalls.push_back({summary, detail});
    updateContent();
  }

private:
  bool isDarkTheme() const
  {
    QPalette pal = QApplication::palette();
    return pal.color(QPalette::Window).lightness() < 128;
  }

  void updateContent()
  {
    QString detailsText;
    for (size_t i = 0; i < toolCalls.size(); ++i) {
      if (i > 0) detailsText += "\n\n";
      detailsText += QString("■ %1\n%2").arg(toolCalls[i].summary, toolCalls[i].detail);
    }
    detailsLabel->setText(detailsText);
    updateButtonText();
  }

  void updateButtonText()
  {
    bool expanded = toggleButton->isChecked();
    QString arrow = expanded ? "▼" : "▶";
    QString text = QString("%1 AI used %2 tool(s)").arg(arrow).arg(toolCalls.size());
    toggleButton->setText(text);
  }

  struct ToolCallLog {
    QString summary;
    QString detail;
  };

  std::vector<ToolCallLog> toolCalls;
  QPushButton *toggleButton;
  QLabel *detailsLabel;
};
