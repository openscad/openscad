#include "gui/ErrorLog.h"

#include <QApplication>
#include <QStyledItemDelegate>
#include <catch2/catch_all.hpp>
#include <string>

namespace {

class CountingDelegate : public QStyledItemDelegate
{
public:
  explicit CountingDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    ++sizeHintCalls;
    return QStyledItemDelegate::sizeHint(option, index);
  }

  mutable int sizeHintCalls{0};
};

}  // namespace

TEST_CASE("Appending warnings only measures the new error log rows", "[gui][error-log]")
{
  int argc = 3;
  char name[] = "error-log-test";
  char platformOption[] = "-platform";
  char platform[] = "offscreen";
  char *argv[] = {name, platformOption, platform, nullptr};
  const QApplication app(argc, argv);
  ErrorLog log;
  log.setGeometry(0, 0, 800, 500);
  log.show();
  QApplication::processEvents();
  auto *delegate = new CountingDelegate(log.logTable);
  log.logTable->setItemDelegate(delegate);

  constexpr int warningCount = 256;
  for (int index = 0; index < warningCount; ++index) {
    log.toErrorLog({"Warning " + std::to_string(index), message_group::Warning, Location::NONE, ""});
  }

  const auto *model = log.logTable->model();
  REQUIRE(model->rowCount() == warningCount);
  CHECK(model->index(0, errorLog_column::message).data().toString() == QStringLiteral("Warning 0"));
  CHECK(model->index(warningCount - 1, errorLog_column::message).data().toString() ==
        QStringLiteral("Warning 255"));
  CHECK(delegate->sizeHintCalls <= 8 * warningCount);

  const int sizeHintsBeforeResize = delegate->sizeHintCalls;
  const int fileColumnWidth = log.logTable->columnWidth(errorLog_column::file);
  log.logTable->setColumnWidth(errorLog_column::file, fileColumnWidth + 20);
  CHECK(delegate->sizeHintCalls >= sizeHintsBeforeResize + warningCount);

  const int sizeHintsAfterResize = delegate->sizeHintCalls;
  log.logTable->horizontalHeader()->sectionResized(errorLog_column::file, fileColumnWidth,
                                                   fileColumnWidth + 20);
  CHECK(delegate->sizeHintCalls == sizeHintsAfterResize);

  log.toErrorLog({"An error", message_group::Error, Location::NONE, ""});
  log.errorLogComboBox->setCurrentIndex(1);
  REQUIRE(log.logTable->model()->rowCount() == 1);
  CHECK(log.logTable->model()->index(0, errorLog_column::message).data().toString() ==
        QStringLiteral("An error"));
  log.errorLogComboBox->setCurrentIndex(0);
  CHECK(log.logTable->model()->rowCount() == warningCount + 1);
  log.clearModel();
  CHECK(log.logTable->model()->rowCount() == 0);
}
