#pragma once

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>

class EditorInterface;
class TabManager;
class MainWindow;

/**
 * A GIMP-style dialog for handling unsaved changes when closing the application.
 *
 * Features:
 * - Shows a list of all unsaved files
 * - Clicking an item activates that tab in the main window and triggers a preview
 * - Each item has a save button that saves the file (or opens Save As for new files)
 * - Saved items are removed from the list
 * - Dialog accepts (allowing close) when all items are saved or "Discard Changes" is clicked
 */
class UnsavedChangesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit UnsavedChangesDialog(TabManager *tabManager, MainWindow *mainWindow,
                                QWidget *parent = nullptr);
  ~UnsavedChangesDialog() override = default;

  enum Result { Cancel = 0, DiscardAll = 1, AllSaved = 2 };

  Result result() const { return dialogResult; }

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
