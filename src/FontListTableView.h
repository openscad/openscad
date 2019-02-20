#pragma once

#include <QTableView>

class FontListTableView : public QTableView
{
	Q_OBJECT;

public:
        FontListTableView(QWidget *parent = nullptr);
        void setDragText(const QString &text);

protected:
        void startDrag(Qt::DropActions supportedActions) override;

private:
        QString text;
};
