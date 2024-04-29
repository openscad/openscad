#pragma once

#include <vector>

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include "qtgettext.h"
#include "ui_FontList.h"

class FontSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FontSortFilterProxyModel(QObject *parent = nullptr);

    void clearFilter();
    void setFilterHashes(const std::vector<uint32_t>&);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QSet<QString> filterHashes;
};

class FontList : public QWidget, public Ui::FontListWidget
{
  Q_OBJECT

public:
  FontList(QWidget *parent = nullptr);
  FontList(const FontList& source) = delete;
  FontList(FontList&& source) = delete;
  FontList& operator=(const FontList& source) = delete;
  FontList& operator=(FontList&& source) = delete;
  ~FontList() override = default;

  void initGUI();
  void update_font_list();

  static constexpr int COL_FONT_NAME = 0;
  static constexpr int COL_FONT_STYLE = 1;
  static constexpr int COL_FILE_NAME = 2;
  static constexpr int COL_HASH = 3;

public slots:
  void on_copyButton_clicked();
  void on_filterLineEdit_textChanged(const QString&);
  void on_charsLineEdit_textChanged(const QString&);
  void selection_changed(const QItemSelection&, const QItemSelection&);

signals:
  void font_selected(const QString font);

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  QString quote(const QString& text);

  QString selection;
  QStandardItemModel *model;
  FontSortFilterProxyModel *proxy;
};
