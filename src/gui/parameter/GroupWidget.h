#pragma once

#include <QGridLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class GroupWidget : public QWidget
{
  Q_OBJECT
private:
  QGridLayout mainLayout;
  QToolButton toggleButton;
  QWidget contentArea;
  QVBoxLayout contentLayout;

public:
  GroupWidget(QString title, QWidget *parent = nullptr);
  void addWidget(QWidget *widget);

  [[nodiscard]] bool isExpanded() const { return toggleButton.isChecked(); }
  [[nodiscard]] QString title() const { return toggleButton.text(); }

public slots:
  void setExpanded(bool expanded);
};
