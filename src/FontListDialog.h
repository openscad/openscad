#pragma once

#include <QStandardItemModel>

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
        void on_pasteButton_clicked();
        void selection_changed(const QItemSelection &, const QItemSelection &);

signals:
        void font_selected(const QString font);

private:
        int selected_row;
        QStandardItemModel *model;
};
