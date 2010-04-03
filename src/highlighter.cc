/*
 *  OpenSCAD (www.openscad.at)
 *  Copyright (C) 2009  Clifford Wolf <clifford@clifford.at>
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

#include "highlighter.h"
#include "openscad.h" // extern int parser_error_pos;

#ifdef _QCODE_EDIT_
Highlighter::Highlighter(QDocument *parent)
#else
Highlighter::Highlighter(QTextDocument *parent)
#endif
		: QSyntaxHighlighter(parent)
{
}

void Highlighter::highlightBlock(const QString &text)
{
	int n = previousBlockState();
	if (n < 0)
		n = 0;
	int k = n + text.size() + 1;
	setCurrentBlockState(k);
	if (parser_error_pos >= n && parser_error_pos < k) {
		QTextCharFormat style;
		style.setBackground(Qt::red);
		setFormat(0, text.size(), style);
#if 0
		style.setBackground(Qt::black);
		style.setForeground(Qt::white);
		setFormat(parser_error_pos - n, 1, style);
#endif
	}
}

