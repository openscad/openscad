#ifndef FONTLISTDIALOG_H_
#define FONTLISTDIALOG_H_

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
private:
        QStandardItemModel *model;
};

#endif
