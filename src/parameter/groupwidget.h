#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>
#include <vector>

class GroupWidget : public QWidget {
	Q_OBJECT
private:
	QGridLayout mainLayout;
	QToolButton toggleButton;
	QWidget contentArea;
	bool *show;

public:
	explicit GroupWidget(bool &show,const QString & title = "", QWidget *parent = nullptr);
	void setContentLayout(QLayout & contentLayout);

private slots:
	void onclicked(bool);
};
