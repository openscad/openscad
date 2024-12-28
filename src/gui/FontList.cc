#include "gui/FontList.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QModelIndex>
#include <QPainter>
#include <QPoint>
#include <QResizeEvent>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <array>
#include <cstdint>
#include <qitemselectionmodel.h>
#include <string>
#include <vector>

#include <QClipboard>
#include <QRegularExpression>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMenu>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <QAction>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QSpinBox>
#include <QLineEdit>

#include "FontCache.h"
#include "utils/printutils.h"

FontItemDelegate::FontItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

int FontItemDelegate::fontSize() const
{
  return _fontSize;
}

void FontItemDelegate::setFontSize(int fontSize)
{
  _fontSize = fontSize;
}

QString FontItemDelegate::text() const
{
  return _text;
}

void FontItemDelegate::setText(const QString& text)
{
  _text = text;
}

void FontItemDelegate::initStyleOption(QStyleOptionViewItem *opt, const QModelIndex &idx) const
{
  QStyledItemDelegate::initStyleOption(opt, idx);

  opt->font.setStyleStrategy(QFont::NoFontMerging);
  opt->font.setPointSize(_fontSize);
  opt->textElideMode = Qt::ElideNone;
}

QWidget *FontItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const
{
  return nullptr;
}

QSize FontItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &idx) const
{
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, idx);

  const auto text = opt.text.isEmpty() ? this->text() : opt.text;

  QFontMetrics fm(opt.font);
  // Using the default font for speed, so adding some extra generous padding
  // This also prevents the columns from getting very wide due to some random
  // font having some extra wide glyphs.
  return {4 * fm.height() + fm.horizontalAdvance(text), fm.height()};
}

void FontItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const
{
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, idx);

  const auto fontName = FontList::colStr(idx, FontList::COL_FONT_NAME);
  const auto fontStyle= FontList::colStr(idx, FontList::COL_FONT_STYLE);

  opt.font.setFamily(fontName);
  opt.font.setStyleName(fontStyle);
  opt.text = text(); // only used if idx points to empty string

  QStyledItemDelegate::paint(painter, opt, idx);
}

FontSortFilterProxyModel::FontSortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

void FontSortFilterProxyModel::clearFilter()
{
  filterHashes.clear();
}

void FontSortFilterProxyModel::appendFilterHashes(const std::vector<uint32_t>& hashes)
{
  for (const auto hash : hashes) {
    filterHashes.insert(QString::number(hash, 16));
  }
}

bool FontSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
  const bool parentResult = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
  if (filterHashes.empty()) {
    return parentResult;
  }

  const auto idx = sourceModel()->index(sourceRow, FontList::COL_HASH, sourceParent);
  const auto &data = sourceModel()->data(idx);
  const bool result = filterHashes.contains(data.toString());
  return parentResult && result;
}

FontList::FontList(QWidget *parent) : QWidget(parent), model(nullptr), proxy(nullptr)
{
  setupUi(this);
  lineEditSampleText->addAction(actionResetSampleText, QLineEdit::TrailingPosition);
  actionResetSampleText->trigger();
  lineEditFontNameSelected->addAction(actionCopyFontName, QLineEdit::TrailingPosition);
  lineEditFontPathSelected->addAction(actionOpenFolder, QLineEdit::TrailingPosition);
  lineEditFontPathSelected->addAction(actionCopyFullPath, QLineEdit::TrailingPosition);
  lineEditFcStyleSelected->addAction(actionCopyStyle, QLineEdit::TrailingPosition);
  spinBoxFontSize->setValue(tableView->fontInfo().pointSize());
  selection_changed({}, {});

  tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested, this, &FontList::customHeaderContexMenuRequested);
}

void FontList::on_lineEditFontName_textChanged(const QString& text)
{
  updateFilter(comboBoxSearchType->currentIndex(), text);
}

void FontList::on_comboBoxSearchType_currentIndexChanged(int idx)
{
  updateFilter(idx, lineEditFontName->text());
}

void FontList::updateFilter(int searchTypeIdx, const QString& text)
{
  const auto regExp = QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);

  switch (searchTypeIdx) {
    case 0:
      proxy->setFilterFixedString(text);
      break;
    case 1:
      proxy->setFilterWildcard(text);
      break;
    default:
      proxy->setFilterRegularExpression(regExp);
      break;
  }
  groupBoxFilter->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));
}

void FontList::on_comboBoxCharFilterType_currentIndexChanged(int idx)
{
  updateCharFilter(idx, lineEditChars->text());
}

void FontList::on_lineEditChars_textChanged(const QString& text)
{
  updateCharFilter(comboBoxCharFilterType->currentIndex(), text);
}

void FontList::updateCharFilter(int charFilterType, const QString& text)
{
  proxy->clearFilter();
  if (text.length() > 0) {
    if (charFilterType == 0) {
      // AND / All
      const auto hashes = FontCache::instance()->filter(text.toStdU32String());
      proxy->appendFilterHashes(hashes);
    } else {
      // OR / Any
      for (const auto ch : text.toStdU32String()) {
        const auto hashes = FontCache::instance()->filter(std::u32string{ch});
        proxy->appendFilterHashes(hashes);
      }
    }
  }
  proxy->invalidate();
  groupBoxFilter->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));
}

void FontList::on_actionResetSampleText_triggered()
{
  lineEditSampleText->setText(SAMPLE_TEXT_DEFAULT);
  lineEditSampleText->setCursorPosition(0);
}

void FontList::on_lineEditSampleText_textChanged(const QString& text)
{
  updateSampleText(text, spinBoxFontSize->value());
}

void FontList::on_spinBoxFontSize_valueChanged(int value)
{
  updateSampleText(lineEditSampleText->text(), value);
}

void FontList::updateSampleText(const QString& text, int fontSize)
{
  delegate.setText(text);
  delegate.setFontSize(fontSize);

  QFont font = tableView->font();
  font.setPointSize(fontSize);
  const QFontMetrics fm(font);
  const int size = fm.height() + fm.height() / 4;
  tableView->verticalHeader()->setMinimumSectionSize(1);
  tableView->verticalHeader()->setMaximumSectionSize(size);
  tableView->verticalHeader()->setDefaultSectionSize(size);
  tableView->resizeColumnToContents(COL_SAMPLE);
  tableView->resizeColumnToContents(COL_STYLED_FONT_NAME);

  if (proxy) {
    proxy->invalidate();
  }
}

void FontList::customHeaderContexMenuRequested(const QPoint& pos)
{
  auto *menu = new QMenu(this);
  menu->addAction(actionShowFontNameColumn);
  menu->addAction(actionShowStyledFontNameColumn);
  menu->addAction(actionShowFontStyleColumn);
  menu->addAction(actionShowFontSampleColumn);
  menu->addAction(actionShowFileNameColumn);
  menu->addAction(actionShowFilePathColumn);
  menu->addSeparator();
  menu->addAction(actionResetColumns);
  menu->popup(tableView->horizontalHeader()->viewport()->mapToGlobal(pos));
}

void FontList::showColumn(int column, bool show) {
  tableView->setColumnHidden(column, !show);
  if (show) {
    tableView->resizeColumnToContents(column);
  }
}

void FontList::on_actionShowFontNameColumn_toggled(bool show)
{
  showColumn(COL_FONT_NAME, show);
}

void FontList::on_actionShowStyledFontNameColumn_toggled(bool show)
{
  showColumn(COL_STYLED_FONT_NAME, show);
}

void FontList::on_actionShowFontStyleColumn_toggled(bool show)
{
  showColumn(COL_FONT_STYLE, show);
}

void FontList::on_actionShowFontSampleColumn_toggled(bool show)
{
  showColumn(COL_SAMPLE, show);
}

void FontList::on_actionShowFileNameColumn_toggled(bool show)
{
  showColumn(COL_FILE_NAME, show);
}

void FontList::on_actionShowFilePathColumn_toggled(bool show)
{
  showColumn(COL_FILE_PATH, show);
}

void FontList::on_actionResetColumns_triggered()
{
  actionShowFontNameColumn->setChecked(true);
  actionShowStyledFontNameColumn->setChecked(false);
  actionShowFontStyleColumn->setChecked(true);
  actionShowFontSampleColumn->setChecked(true);
  actionShowFileNameColumn->setChecked(true);
  actionShowFilePathColumn->setChecked(false);
  // depending on the initial state of the action, the
  // toggle callback might not trigger, so force the sync
  // of the column state
  tableView->hideColumn(COL_STYLED_FONT_NAME);
  tableView->hideColumn(COL_FILE_PATH);
  tableView->hideColumn(COL_HASH);
}

void FontList::on_tableView_customContextMenuRequested(const QPoint& pos)
{
  auto *menu = new QMenu(this);
  menu->addAction(actionCopyStyle);
  menu->addSeparator();
  menu->addAction(actionCopyFontName);
  menu->addAction(actionCopyFolder);
  menu->addAction(actionCopyFullPath);
  menu->addSeparator();
  menu->addAction(actionOpenFolder);
  menu->popup(tableView->viewport()->mapToGlobal(pos));
}

const QModelIndex FontList::currentIndex() const
{
  return tableView->selectionModel()->currentIndex();
}

void FontList::on_actionCopyFontName_triggered()
{
  QApplication::clipboard()->setText(colStr(currentIndex(), COL_FONT_NAME));
}

void FontList::on_actionCopyStyle_triggered()
{
  font_selected(selection);
  QApplication::clipboard()->setText(selection);
}

void FontList::on_actionCopyFolder_triggered()
{
  const QFileInfo fileInfo(colStr(currentIndex(), COL_FILE_PATH));
  QApplication::clipboard()->setText(fileInfo.dir().canonicalPath());
}

void FontList::on_actionCopyFullPath_triggered()
{
  QApplication::clipboard()->setText(colStr(currentIndex(), COL_FILE_PATH));
}

void FontList::on_actionOpenFolder_triggered()
{
  const QFileInfo fileInfo(colStr(currentIndex(), COL_FILE_PATH));
  if (fileInfo.dir().exists()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().absolutePath()));
  }
}

void FontList::selection_changed(const QItemSelection& current, const QItemSelection&)
{
  const std::array<QAction *, 5> actions = {
    actionCopyFontName,
    actionCopyStyle,
    actionCopyFolder,
    actionCopyFullPath,
    actionOpenFolder,
  };

  const bool disabled = current.count() == 0;
  for (const auto action : actions) {
    action->setDisabled(disabled);
    action->setVisible(!disabled);
  }

  if (disabled) {
    tableView->setDragText("");
    lineEditFontNameSelected->setText("");
    lineEditFontPathSelected->setText("");
    lineEditFcStyleSelected->setText("");
    return;
  }

  const auto& idx = proxy->mapToSource(current.indexes().at(0));
  const auto name = model->item(idx.row(), COL_FONT_NAME)->text();
  const auto path = model->item(idx.row(), COL_FILE_PATH)->text();
  const auto style = model->item(idx.row(), COL_FONT_STYLE)->text();
  const auto fcStyle = QString("%1:style=%2").arg(quote(name)).arg(quote(style));
  this->selection = "\"" + fcStyle + "\"";
  tableView->setDragText(this->selection);
  lineEditFontNameSelected->setText(name);
  lineEditFontNameSelected->setCursorPosition(0);
  lineEditFontPathSelected->setText(path); // keep cursor at the end to prefer the file name
  lineEditFcStyleSelected->setText(fcStyle);
  lineEditFcStyleSelected->setCursorPosition(0);
}

void FontList::update_font_list()
{
  if (proxy) {
    delete proxy;
    proxy = nullptr;
  }
  if (model) {
    delete model;
    model = nullptr;
  }

  const FontInfoList *list = FontCache::instance()->list_fonts();
  model = new QStandardItemModel(list->size(), COL_COUNT, this);
  model->setHorizontalHeaderItem(COL_FONT_NAME, new QStandardItem(_("Font name")));
  model->setHorizontalHeaderItem(COL_STYLED_FONT_NAME, new QStandardItem(_("Styled font name")));
  model->setHorizontalHeaderItem(COL_FONT_STYLE, new QStandardItem(_("Font style")));
  model->setHorizontalHeaderItem(COL_SAMPLE, new QStandardItem(_("Sample text"))); // text handled by delegate
  model->setHorizontalHeaderItem(COL_FILE_NAME, new QStandardItem(_("File name")));
  model->setHorizontalHeaderItem(COL_FILE_PATH, new QStandardItem(_("File path")));
  model->setHorizontalHeaderItem(COL_HASH, new QStandardItem(_("Hash")));

  const QString toolTip = R"(<table style="white-space:pre"><tr><td>Name:</td><td><b>%1</b></td></tr><tr><td>Style:</td><td>%2</td></tr></table>)";

  int idx = 0;
  for (auto it = list->begin(); it != list->end(); it++, idx++) {
    const FontInfo& font_info = (*it);

    auto *family = new QStandardItem(QString::fromStdString(font_info.get_family()));
    family->setEditable(false);
    model->setItem(idx, COL_FONT_NAME, family);

    auto *style = new QStandardItem(QString::fromStdString(font_info.get_style()));
    style->setEditable(false);
    model->setItem(idx, COL_FONT_STYLE, style);

    auto *styledFamily = new QStandardItem(QString::fromStdString(font_info.get_family()));
    styledFamily->setEditable(false);
    styledFamily->setToolTip(toolTip.arg(family->text()).arg(style->text()));
    model->setItem(idx, COL_STYLED_FONT_NAME, styledFamily);

    const QFileInfo fileInfo(QString::fromStdString(font_info.get_file()));
    auto *file = new QStandardItem(fileInfo.fileName());
    file->setEditable(false);
    file->setToolTip(fileInfo.canonicalFilePath());
    model->setItem(idx, COL_FILE_NAME, file);
    auto *filePath = new QStandardItem(fileInfo.canonicalFilePath());
    filePath->setEditable(false);
    model->setItem(idx, COL_FILE_PATH, filePath);

    auto *hash = new QStandardItem(QString::number(font_info.get_hash(), 16));
    hash->setEditable(false);
    model->setItem(idx, COL_HASH, hash);
  }

  proxy = new FontSortFilterProxyModel(this);
  proxy->setSourceModel(model);
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  groupBoxFilter->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));

  this->tableView->setModel(proxy);
  on_actionResetColumns_triggered();
  this->tableView->setItemDelegateForColumn(COL_STYLED_FONT_NAME, &delegate);
  this->tableView->setItemDelegateForColumn(COL_SAMPLE, &delegate);
  this->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->tableView->sortByColumn(COL_FONT_NAME, Qt::AscendingOrder);
  this->tableView->resizeColumnsToContents();
  this->tableView->setSortingEnabled(true);

  connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FontList::selection_changed);

  delete list;
}

void FontList::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
}

/**
 * Quote a string according to the requirements of font-config.
 * See http://www.freedesktop.org/software/fontconfig/fontconfig-user.html
 *
 * The '\', '-', ':' and ',' characters in family names must be preceded
 * by a '\' character to avoid having them misinterpreted. Similarly, values
 * containing '\', '=', '_', ':' and ',' must also have them preceded by a
 * '\' character. The '\' characters are stripped out of the family name and
 * values as the font name is read.
 *
 * @param text unquoted string
 * @return quoted text
 */
QString FontList::quote(const QString& text)
{
  QString result = text;
  result.replace('\\', R"(\\\\)")
  .replace('-', "\\\\-")
  .replace(':', "\\\\:")
  .replace(',', "\\\\,")
  .replace('=', "\\\\=")
  .replace('_', "\\\\_");
  return result;
}

const QModelIndex FontList::colIdx(const QModelIndex& idx, int column)
{
  return idx.model()->index(idx.row(), column, idx.parent());
}

const QString FontList::colStr(const QModelIndex& idx, int column)
{
  return idx.model() ?
    idx.model()->data(colIdx(idx, column)).toString() :
    QString{};
}
