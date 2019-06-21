#include <QTabBar>
#include <QStackedWidget>
#include <QList>

#include "tabwidget.h"

TabWidget::TabWidget(QWidget *parent) : QTabBar(parent)
{
	stackWidget = new QStackedWidget(this);

	connect(this, SIGNAL(currentChanged(int)), this, SLOT(handleCurrentChanged(int)));
	connect(this, SIGNAL(tabMoved(int, int)), this, SLOT(handleTabMoved(int, int)));
}

TabWidget::~TabWidget()
{
}

QWidget *TabWidget::getContentWidget()
{
	return stackWidget;
}

int TabWidget::addTab(QWidget *w, const QString &label)
{
	stackWidget->addWidget(w);
	int idx = tabContent.size();
	tabContent.insert(idx, w);
	int i = this->insertTab(idx, label);
	fireTabCountChanged();
	return i;
}

void TabWidget::fireTabCountChanged()
{
	emit tabCountChanged(this->count());
}

void TabWidget::handleCurrentChanged(int i)
{
	stackWidget->setCurrentWidget(tabContent.at(i));
	emit currentTabChanged(i);
}

void TabWidget::handleTabMoved(int from, int to)
{
	tabContent.move(from, to);
}

void TabWidget::setCurrentWidget(int index)
{
	QTabBar::setCurrentIndex(index);
}

int TabWidget::indexOf(QWidget *w)
{
	return tabContent.indexOf(w);
}

QWidget *TabWidget::widget(int index)
{
	return tabContent.at(index);
}

void TabWidget::removeTab(int index)
{
	stackWidget->removeWidget(tabContent.at(index));
	tabContent.removeAt(index);
	QTabBar::removeTab(index);
}
