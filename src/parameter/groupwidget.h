#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class GroupWidget : public QWidget {
	Q_OBJECT
private:
	QGridLayout mainLayout;
	QToolButton toggleButton;
	QWidget contentArea;
	bool *show; //pointer to the show flag in the group map

public:
	explicit GroupWidget(bool &show,const QString & title = "", QWidget *parent = nullptr);
	void setContentLayout(QLayout & contentLayout);

private slots:
	void onclicked(bool);
};
