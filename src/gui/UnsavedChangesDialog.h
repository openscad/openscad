#pragma once

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QPushButton>

class EditorInterface;
class TabManager;
class MainWindow;

// GIMP-style dialog for handling unsaved changes on close.
class UnsavedChangesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit UnsavedChangesDialog(TabManager *tabManager, MainWindow *mainWindow,
                                QWidget *parent = nullptr);
  ~UnsavedChangesDialog() override = default;

  enum Result { Cancel = 0, DiscardAll = 1, AllSaved = 2 };

  Result unsavedResult() const { return dialogResult; }

private slots:
  void onItemClicked(QListWidgetItem *item);
  void onSaveButtonClicked();
  void onDiscardClicked();

private:
  void populateList();
  void updateListItem(EditorInterface *editor);
  void removeListItem(EditorInterface *editor);
  void updateCountLabel();
  QWidget *createItemWidget(EditorInterface *editor);
  QString getDisplayName(EditorInterface *editor) const;

  TabManager *tabManager;
  MainWindow *mainWindow;
  QListWidget *listWidget;
  QLabel *countLabel;
  QPushButton *discardButton;
  QPushButton *cancelButton;

  QMap<QListWidgetItem *, EditorInterface *> itemToEditor;
  QMap<EditorInterface *, QListWidgetItem *> editorToItem;

  Result dialogResult;
};
