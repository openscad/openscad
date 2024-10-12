#pragma once

#include <QStringList>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextEdit>
#include "core/IndicatorData.h"
#include "gui/parameter/ParameterWidget.h"

#include <string>
#include <vector>

enum class EditorSelectionIndicatorStatus
{
  SELECTED,
  IMPACTED
};

class EditorInterface : public QWidget
{
  Q_OBJECT
public:
  EditorInterface(QWidget *parent) : QWidget(parent) {}
  QSize sizeHint() const override { QSize size; return size;}
  virtual void setInitialSizeHint(const QSize&) { }
  void wheelEvent(QWheelEvent *) override;
  virtual QString toPlainText() = 0;
  virtual QTextDocument *document(){auto *t = new QTextDocument; return t;}
  virtual QString selectedText() = 0;
  virtual int updateFindIndicators(const QString& findText, bool visibility = true) = 0;
  virtual bool find(const QString&, bool findNext = false, bool findBackwards = false) = 0;
  virtual void replaceSelectedText(const QString& newText) = 0;
  virtual void replaceAll(const QString& findText, const QString& replaceText) = 0;
  virtual QStringList colorSchemes() = 0;
  virtual bool canUndo() = 0;
  virtual void addTemplate() = 0;
  virtual void resetHighlighting() = 0;
  virtual void setIndicator(const std::vector<IndicatorData>& indicatorData) = 0;
  virtual QMenu *createStandardContextMenu() = 0;
  virtual QPoint mapToGlobal(const QPoint&) = 0;
  virtual void setCursorPosition(int /*line*/, int /*col*/) {}
  virtual void setFocus() = 0;

signals:
  void contentsChanged();
  void modificationChanged(EditorInterface *);
  void showContextMenuEvent(const QPoint& pos);
  void focusIn();

public slots:
  virtual void zoomIn() = 0;
  virtual void zoomOut() = 0;
  virtual void setContentModified(bool) = 0;
  virtual bool isContentModified() = 0;
  virtual void indentSelection() = 0;
  virtual void unindentSelection() = 0;
  virtual void commentSelection() = 0;
  virtual void uncommentSelection() = 0;
  virtual void setPlainText(const QString&) = 0;
  virtual void setSelectionIndicatorStatus(EditorSelectionIndicatorStatus status, int level, int lineFrom, int colFrom, int lineTo, int colTo) = 0;
  virtual void clearAllSelectionIndicators() = 0;
  virtual void highlightError(int) = 0;
  virtual void unhighlightLastError() = 0;
  virtual void setHighlightScheme(const QString&) = 0;
  virtual void insert(const QString&) = 0;
  virtual void setText(const QString&) = 0;
  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual void cut() = 0;
  virtual void copy() = 0;
  virtual void paste() = 0;
  virtual void initFont(const QString&, uint) = 0;
  virtual void displayTemplates() = 0;
  virtual void foldUnfold() = 0;
  virtual void toggleBookmark() = 0;
  virtual void nextBookmark() = 0;
  virtual void prevBookmark() = 0;
  virtual void jumpToNextError() = 0;

private:
  QSize initialSizeHint;

public:
  bool contentsRendered; // Set if the source code has changes since the last render (F6)
  int findState;
  QString filepath;
  std::string autoReloadId;
  std::vector<IndicatorData> indicatorData;
  ParameterWidget *parameterWidget;
};
