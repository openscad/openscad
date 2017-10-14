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
	QFrame headerLine;
	QParallelAnimationGroup toggleAnimation;
	QScrollArea contentArea;
	int animationDuration;
	bool *show;
public:
	explicit GroupWidget(bool &show,const QString & title = "", const int animationDuration = 0, QWidget *parent = 0);
	void setContentLayout(QLayout & contentLayout);
																								
private slots:
	void onclicked(bool);
};
