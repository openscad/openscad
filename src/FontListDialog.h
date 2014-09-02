#pragma once

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "ui_FontListDialog.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

class FontListDialog : public QDialog, public Ui::FontListDialog
{
	Q_OBJECT;
public:
	FontListDialog();
        virtual ~FontListDialog();
        
        void update_font_list();

public slots:
        void on_copyButton_clicked();
        void on_filterLineEdit_textChanged(const QString &);
        void selection_changed(const QItemSelection &, const QItemSelection &);

signals:
        void font_selected(const QString font);

private:
        QString selection;
        QStandardItemModel *model;
        QSortFilterProxyModel *proxy;
};
