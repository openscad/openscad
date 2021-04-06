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

#pragma once

#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QString>
#include <vector>
#include "qtgettext.h"
#include "ui_Console.h"

struct ConsoleMessageBlock {
	QString message;
	QString link;
	message_group group;
};

class Console : public QPlainTextEdit, public Ui::Console
{
	Q_OBJECT

private:
	static constexpr int MAX_LINES = 5000;
	std::vector<ConsoleMessageBlock> msgBuffer;
	QTextCursor appendCursor; // keep a cursor always at the end of document.

public:
	Console(QWidget *parent = nullptr);
	virtual ~Console();
	QString clickedAnchor;
	void contextMenuEvent(QContextMenuEvent *event) override;
	
	void mousePressEvent(QMouseEvent *e) override
	{
		clickedAnchor = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : QString();
		QPlainTextEdit::mousePressEvent(e);
	}

	void mouseReleaseEvent(QMouseEvent *e) override
	{
		if (e->button() & Qt::LeftButton && !clickedAnchor.isEmpty() &&
				anchorAt(e->pos()) == clickedAnchor) {
			emit linkActivated(clickedAnchor);
		}

		QPlainTextEdit::mouseReleaseEvent(e);
	}

	void addMessage(const Message &msg);
	void addHtml(const QString &html);

signals:
	void linkActivated(QString);
	void openFile(QString,int);

public slots:
	void actionClearConsole_triggered();
	void actionSaveAs_triggered();
	void hyperlinkClicked(const QString& loc);
	void setFont(const QString &fontFamily, uint ptSize);
	void update();
};
