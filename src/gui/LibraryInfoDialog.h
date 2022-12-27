#pragma once

#include <QDialog>
#include <QString>

#include "qtgettext.h" // IWYU pragma: keep
#include "ui_LibraryInfoDialog.h"

class LibraryInfoDialog : public QDialog, public Ui::LibraryInfoDialog
{
  Q_OBJECT;

public:
  LibraryInfoDialog(const QString& rendererInfo);

  void update_library_info(const QString& rendererInfo);
};
