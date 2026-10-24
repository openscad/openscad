#pragma once

#include <QGridLayout>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class GroupDisclosureButton : public QToolButton
{
public:
  explicit GroupDisclosureButton(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
};

class GroupWidget : public QWidget
{
  Q_OBJECT
private:
  QGridLayout mainLayout;
  GroupDisclosureButton toggleButton;
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
