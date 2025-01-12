#pragma once

#include <QString>
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
  GroupWidget(const QString& title, QWidget *parent = nullptr);
  void addWidget(QWidget *widget);

  bool isExpanded() const { return toggleButton.isChecked(); }
  QString title() const { return toggleButton.text(); }

public slots:
  void setExpanded(bool expanded);
};
