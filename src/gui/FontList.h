#pragma once

#include <QItemSelection>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>
#include <cstdint>
#include <vector>

#include <QWidget>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>

#include "gui/qtgettext.h"
#include "ui_FontList.h"

class FontItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  explicit FontItemDelegate(QObject *parent = nullptr);
  int fontSize() const;
  void setFontSize(int fontSize);
  QString text() const;
  void setText(const QString&);
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
  void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
  int _fontSize;
  QString _text;
};

class FontSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FontSortFilterProxyModel(QObject *parent = nullptr);

    void clearFilter();
    void appendFilterHashes(const std::vector<uint32_t>&);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QSet<QString> filterHashes;
};

class FontList : public QWidget, public Ui::FontListWidget
{
  Q_OBJECT
  static constexpr char SAMPLE_TEXT_DEFAULT[] = "abcdef ABCDEF 012345 0O5S8B 1Iil !$%&/()#";

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
  static constexpr int COL_STYLED_FONT_NAME = 1;
  static constexpr int COL_FONT_STYLE = 2;
  static constexpr int COL_SAMPLE = 3;
  static constexpr int COL_FILE_NAME = 4;
  static constexpr int COL_FILE_PATH = 5;
  static constexpr int COL_HASH = 6;
  static constexpr int COL_COUNT = 7;

public slots:
  void on_lineEditFontName_textChanged(const QString&);
  void on_comboBoxSearchType_currentIndexChanged(int);
  void updateFilter(int, const QString&);

  void on_comboBoxCharFilterType_currentIndexChanged(int);
  void on_lineEditChars_textChanged(const QString&);
  void updateCharFilter(int, const QString&);

  void on_actionResetSampleText_triggered();
  void on_lineEditSampleText_textChanged(const QString&);
  void on_spinBoxFontSize_valueChanged(int);
  void updateSampleText(const QString&, int);

  void customHeaderContexMenuRequested(const QPoint&);
  void on_tableView_customContextMenuRequested(const QPoint&);
  void on_actionCopyFontName_triggered();
  void on_actionCopyStyle_triggered();
  void on_actionCopyFolder_triggered();
  void on_actionCopyFullPath_triggered();
  void on_actionOpenFolder_triggered();
  void selection_changed(const QItemSelection&, const QItemSelection&);

  void showColumn(int, bool);
  void on_actionShowFontNameColumn_toggled(bool);
  void on_actionShowStyledFontNameColumn_toggled(bool);
  void on_actionShowFontStyleColumn_toggled(bool);
  void on_actionShowFontSampleColumn_toggled(bool);
  void on_actionShowFileNameColumn_toggled(bool);
  void on_actionShowFilePathColumn_toggled(bool);
  void on_actionResetColumns_triggered();

  static const QModelIndex colIdx(const QModelIndex& idx, int column);
  static const QString colStr(const QModelIndex& idx, int column);

signals:
  void font_selected(const QString font);

protected:
  const QModelIndex currentIndex() const;
  void resizeEvent(QResizeEvent *event) override;

private:
  QString quote(const QString& text);

  QString selection;
  FontItemDelegate delegate;
  QStandardItemModel *model;
  FontSortFilterProxyModel *proxy;
};
