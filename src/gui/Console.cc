/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2019 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "gui/Console.h"

#include <QBrush>
#include <QColor>
#include <QContextMenuEvent>
#include <QFocusEvent>
#include <QPlainTextEdit>
#include <QStringLiteral>
#include <QTextCharFormat>
#include <QWidget>
#include <cassert>
#include <QMenu>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QRegularExpression>
#include <QString>
#include "gui/MainWindow.h"
#include "utils/printutils.h"
#include "gui/Preferences.h"
#include "gui/UIUtils.h"

Console::Console(QWidget *parent) : QPlainTextEdit(parent)
{
  setupUi(this);
  connect(this->actionClear, SIGNAL(triggered()), this, SLOT(actionClearConsole_triggered()));
  connect(this->actionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs_triggered()));
  connect(this, SIGNAL(linkActivated(QString)), this, SLOT(hyperlinkClicked(const QString&)));
  this->setUndoRedoEnabled(false);
  this->appendCursor = this->textCursor();
}

void Console::focusInEvent(QFocusEvent * /*event*/)
{
  QWidget *current = this;
  MainWindow *mw;
  while (current && !(mw = dynamic_cast<MainWindow *>(current->window()))) {
    current = current->parentWidget();
  }
  assert(mw);
  if (mw) mw->setLastFocus(this);
}

void Console::addMessage(const Message& msg)
{
  // Messages with links to source must be inserted separately,
  // since anchor href is set via the "format" argument of:
  //    QTextCursor::insertText(const QString &text, const QTextCharFormat &format)
  // But if no link, and matching colors, then concat message strings with newline in between.
  // This results in less calls to insertText in Console::update(), and much better performance.
  if (!this->msgBuffer.empty() && msg.loc.isNone() && this->msgBuffer.back().link.isEmpty() &&
      (getGroupColor(msg.group) == getGroupColor(this->msgBuffer.back().group)) ) {
    auto& lastmsg = this->msgBuffer.back().message;
    lastmsg += QChar('\n');
    lastmsg += QString::fromStdString(msg.str());
  } else {
    this->msgBuffer.push_back(
    {
      QString::fromStdString(msg.str()),
      (getGroupTextPlain(msg.group) || msg.loc.isNone()) ?
      QString() :
      QString("%1,%2").arg(msg.loc.firstLine()).arg(QString::fromStdString(msg.loc.fileName())),
      msg.group
    }
      );
  }
}

// Slow due to HTML parsing required, only used for initial Console header.
void Console::addHtml(const QString& html)
{
  this->appendHtml(html + QStringLiteral("&nbsp;"));
  this->appendCursor.movePosition(QTextCursor::End);
  this->setTextCursor(this->appendCursor);
}

void Console::setFont(const QString& fontFamily, uint ptSize) {
  this->document()->setDefaultFont(QFont(fontFamily, ptSize));
}

void Console::update()
{
  // Faster to ignore block count until group of messages are done inserting.
  this->setMaximumBlockCount(0);
  for (const auto& line : this->msgBuffer) {
    QTextCharFormat charFormat;
    if (line.group != message_group::NONE && line.group != message_group::Echo) charFormat.setForeground(QBrush(QColor("#000000")));
    charFormat.setBackground(QBrush(QColor(getGroupColor(line.group).c_str())));
    if (!line.link.isEmpty()) {
      charFormat.setAnchor(true);
      charFormat.setAnchorHref(line.link);
      charFormat.setFontUnderline(true);
    }
    // TODO insert timestamp as tooltip? (see #3570)
    //   may have to get rid of concatenation feature of Console::addMessage,
    //   or just live with grouped messages using the same timestamp
    //charFormat.setToolTip(timestr);

    appendCursor.insertBlock();
    appendCursor.insertText(line.message, charFormat);
  }
  msgBuffer.clear();
  this->setTextCursor(appendCursor);
  this->setMaximumBlockCount(Preferences::inst()->getValue("advanced/consoleMaxLines").toUInt());
}

void Console::actionClearConsole_triggered()
{
  this->msgBuffer.clear();
  this->document()->clear();
  this->appendCursor = this->textCursor();
}

void Console::actionSaveAs_triggered()
{
  const auto& text = this->document()->toPlainText();
  const auto fileName = QFileDialog::getSaveFileName(this, _("Save console content"));
  QFile file(fileName);
  if (file.open(QIODevice::ReadWrite)) {
    QTextStream stream(&file);
    stream << text;
    stream.flush();
    LOG("Console content saved to '%1$s'.", fileName.toStdString());
  }
}

void Console::contextMenuEvent(QContextMenuEvent *event)
{
  // Clear leaves characterCount() at 1, not 0
  const bool hasContent = this->document()->characterCount() > 1;
  this->actionClear->setEnabled(hasContent);
  this->actionSaveAs->setEnabled(hasContent);
  QMenu *menu = createStandardContextMenu();
  menu->insertAction(menu->actions().at(0), this->actionClear);
  menu->addSeparator();
  menu->addAction(this->actionSaveAs);
  menu->exec(event->globalPos());
  delete menu;
}

void Console::hyperlinkClicked(const QString& url)
{
  if (url.startsWith("http://") || url.startsWith("https://")) {
    UIUtils::openURL(url);
    return;
  }

  const QRegularExpression regEx("^(\\d+),(.*)$");
  const auto match = regEx.match(url);
  if (match.hasMatch()) {
    const auto line = match.captured(1).toInt();
    const auto file = match.captured(2);
    const auto info = QFileInfo(file);
    if (info.isFile()) {
      if (info.isReadable()) {
        emit openFile(file, line - 1);
      }
    } else {
      openFile(QString(), line - 1);
    }
  }
}
