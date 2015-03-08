#pragma once

#include <QListWidget>

class LibraryListWidget : public QListWidget
{
	Q_OBJECT;

public:
        LibraryListWidget(QWidget *parent = NULL);
        void setDragText(const QString &text);

protected:
        void startDrag(Qt::DropActions supportedActions);

private:
        QString text;
};
