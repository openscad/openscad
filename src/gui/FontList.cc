#include <QClipboard>

#include "FontList.h"
#include "FontCache.h"
#include "printutils.h"

FontSortFilterProxyModel::FontSortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

void FontSortFilterProxyModel::clearFilter()
{
  filterHashes.clear();
  invalidateFilter();
}

void FontSortFilterProxyModel::setFilterHashes(const std::vector<uint32_t>& hashes)
{
  filterHashes.clear();
  for (const auto hash : hashes) {
    filterHashes.insert(QString::number(hash, 16));
  }
  invalidateFilter();
}

bool FontSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
  if (filterHashes.empty()) {
    return true;
  }

  QModelIndex idx = sourceModel()->index(sourceRow, FontList::COL_HASH, sourceParent);
  const auto &data = sourceModel()->data(idx);
  const bool result = filterHashes.contains(data.toString());
  return result;
}

FontList::FontList(QWidget *parent) : QWidget(parent), model(nullptr), proxy(nullptr)
{
  setupUi(this);
}

void FontList::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
}

void FontList::on_copyButton_clicked()
{
  font_selected(selection);

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(selection);
}

void FontList::on_filterLineEdit_textChanged(const QString& text)
{
  proxy->setFilterWildcard(text);
  groupBox->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));
}

void FontList::on_charsLineEdit_textChanged(const QString& text)
{
  if (text.length() == 0) {
    proxy->clearFilter();
  } else {
    const auto hashes = FontCache::instance()->filter(text.toStdU32String());
    proxy->setFilterHashes(hashes);
  }
  groupBox->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));
}

void FontList::selection_changed(const QItemSelection& current, const QItemSelection&)
{
  if (current.count() == 0) {
    copyButton->setEnabled(false);
    tableView->setDragText("");
    return;
  }

  const QModelIndex& idx = proxy->mapToSource(current.indexes().at(0));
  const QString name = model->item(idx.row(), COL_FONT_NAME)->text();
  const QString style = model->item(idx.row(), COL_FONT_STYLE)->text();
  selection = QString("\"%1:style=%2\"").arg(quote(name)).arg(quote(style));
  copyButton->setEnabled(true);
  tableView->setDragText(selection);
}

void FontList::update_font_list()
{
  copyButton->setEnabled(false);

  if (proxy) {
    delete proxy;
    proxy = nullptr;
  }
  if (model) {
    delete model;
    model = nullptr;
  }

  FontInfoList *list = FontCache::instance()->list_fonts();
  model = new QStandardItemModel(list->size(), 4, this);
  model->setHorizontalHeaderItem(COL_FONT_NAME, new QStandardItem(_("Font name")));
  model->setHorizontalHeaderItem(COL_FONT_STYLE, new QStandardItem(_("Font style")));
  model->setHorizontalHeaderItem(COL_FILE_NAME, new QStandardItem(_("Filename")));
  model->setHorizontalHeaderItem(COL_HASH, new QStandardItem(_("Hash")));

  int idx = 0;
  for (auto it = list->begin(); it != list->end(); it++, idx++) {
    FontInfo font_info = (*it);
    auto *family = new QStandardItem(QString::fromStdString(font_info.get_family()));
    family->setEditable(false);
    model->setItem(idx, COL_FONT_NAME, family);
    auto *style = new QStandardItem(QString::fromStdString(font_info.get_style()));
    style->setEditable(false);
    model->setItem(idx, COL_FONT_STYLE, style);
    auto *file = new QStandardItem(QString::fromStdString(font_info.get_file()));
    file->setEditable(false);
    model->setItem(idx, COL_FILE_NAME, file);
    auto *hash = new QStandardItem(QString::number(font_info.get_hash(), 16));
    hash->setEditable(false);
    model->setItem(idx, COL_HASH, hash);
  }

  proxy = new FontSortFilterProxyModel(this);
  proxy->setSourceModel(model);
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  groupBox->setTitle(QString("Filter (%1 fonts found)").arg(proxy->rowCount()));

  this->tableView->setModel(proxy);
  this->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->tableView->sortByColumn(COL_FONT_NAME, Qt::AscendingOrder);
  this->tableView->resizeColumnsToContents();
  this->tableView->setSortingEnabled(true);

  connect(tableView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)), this, SLOT(selection_changed(const QItemSelection&,const QItemSelection&)));

  delete list;
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
