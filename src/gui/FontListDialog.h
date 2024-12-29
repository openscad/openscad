#pragma once

#include <QDialog>
#include <QItemSelection>
#include <QString>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "gui/qtgettext.h"
#include "ui_FontListDialog.h"

class FontListDialog : public QDialog, public Ui::FontListDialog
{
  Q_OBJECT;
public:
  FontListDialog();

  void update_font_list();

public slots:
  void on_copyButton_clicked();
  void on_filterLineEdit_textChanged(const QString&);
  void selection_changed(const QItemSelection&, const QItemSelection&);

signals:
  void font_selected(const QString font);

private:
  QString quote(const QString& text);

  QString selection;
  QStandardItemModel *model;
  QSortFilterProxyModel *proxy;
};
